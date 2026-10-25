/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2007-2026  Ingo Korb <ingo@akana.de>

   Inspired by MMC2IEC by Lars Pontoppidan et al.

   FAT filesystem access based on code from ChaN and Jim Brain, see ff.c|h.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License only.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


   eeprom-conf.c: Persistent configuration storage

*/

#include <string.h>
#include "config.h"
#include "arch-eeprom.h"
#include "diskio.h"
#include "fatops.h"
#include "flags.h"
#include "iec.h"
#include "bus.h"
#include "timer.h"
#include "ustring.h"
#include "uart.h"
#include "eeprom-conf.h"
#ifdef CONFIG_LCD_WS0010
#  include "display_lcd.h"
#endif
#ifdef CONFIG_ONBOARD_DISPLAY
#  include "lcd-onboard.h"
#  include "menu.h"
#endif

uint8_t rom_filename[ROM_NAME_LENGTH+1];

/**
 * struct storedconfig - in-eeprom data structure
 * @dummy      : EEPROM position 0 is unused
 * @checksum   : Checksum over the EEPROM contents
 * @structsize : size of the eeprom structure
 * @unused     : unused byte kept for structure compatibility
 * @globalflags: subset of the globalflags variable
 * @address    : device address set by software
 * @hardaddress: device address set by jumpers
 * @fileexts   : file extension mapping mode, bit 6 type check, bit 7 lock
 * @drvflags0  : 16 bits of drv mappings, organized as 4 nybbles.
 * @drvflags1  : 16 bits of drv mappings, organized as 4 nybbles.
 * @imagedirs  : Disk images-as-directory mode
 * @romname    : M-R rom emulation file name (zero-padded, but not terminated)
 * @active_bus : IEC or IEEE488
 * @menu_system_enabled : the buttons control the menu or set the device address
 * @lcd_contrast, @lcd_brightness : settings of the on-board display
 * @oled_dim   : seconds until the OLED dims, 0 always off, 255 always on
 *
 * This is the data structure for the contents of the EEPROM.
 *
 * Do not remove any fields!
 * Only add fields at the end!
 */
static EEMEM struct {
  uint8_t  dummy;
  uint8_t  checksum;
  uint16_t structsize;
  uint8_t  unused;
  uint8_t  global_flags;
  uint8_t  address;
  uint8_t  hardaddress;
  uint8_t  fileexts;
  uint16_t drvconfig0;
  uint16_t drvconfig1;
  uint8_t  imagedirs;
  uint8_t  romname[ROM_NAME_LENGTH];
#ifdef CONFIG_ONBOARD_DISPLAY
  uint8_t  active_bus;
  uint8_t  menu_system_enabled;
  uint8_t  lcd_contrast;
  uint8_t  lcd_brightness;
#endif
#ifdef CONFIG_LCD_WS0010
  uint8_t  oled_dim;
#endif
} __attribute__((packed)) storedconfig;

/**
 * read_configuration - reads configuration from EEPROM
 *
 * This function reads the stored configuration values from the EEPROM.
 * If the stored checksum doesn't match the calculated one nothing will
 * be changed.
 */
#ifdef CONFIG_ONBOARD_DISPLAY
bool eeprom_config_ignored = false;
#endif

void read_configuration(void) {
  uint_fast16_t i,size;
  uint8_t checksum, tmp;
#ifdef CONFIG_ONBOARD_DISPLAY
  bool rewrite_required = false;
  /* active_bus reflects the last user selection once the system is up, */
  /* only the first (power-on) call may load it from the EEPROM         */
  static bool active_bus_from_eeprom = true;
  bool load_active_bus = active_bus_from_eeprom;
  active_bus_from_eeprom = false;
#endif

  /* Set default values */
  globalflags         |= POSTMATCH;            /* Post-* matching enabled */
  file_extension_mode  = 1;                    /* Store x00 extensions except for PRG */
  set_drive_config(get_default_driveconfig()); /* Set the default drive configuration */
  memset(rom_filename, 0, sizeof(rom_filename));

#ifdef CONFIG_ONBOARD_DISPLAY
  /* Failsafe: PREV and NEXT held at power-on (see main.c) keep the */
  /* defaults and leave the EEPROM alone                            */
  if (eeprom_config_ignored)
    return;
#else
  /* Use the NEXT button to skip reading the EEPROM configuration */
  if (!(buttons_read() & BUTTON_NEXT)) {
    set_key(IGNORE_KEYS);
    return;
  }
#endif

  size = eeprom_read_word(&storedconfig.structsize);

  /* abort if the size bytes are not set */
  if (size == 0xffff) {
#ifdef CONFIG_ONBOARD_DISPLAY
    write_configuration();
#else
    eeprom_safety();
#endif
    return;
  }

  /* Calculate checksum of EEPROM contents */
  checksum = 0;
  for (i=2; i<size; i++)
    checksum += eeprom_read_byte((uint8_t *)i);

  /* Abort if the checksum doesn't match */
  if (checksum != eeprom_read_byte(&storedconfig.checksum)) {
#ifdef CONFIG_ONBOARD_DISPLAY
    write_configuration();
#else
    eeprom_safety();
#endif
    return;
  }

  /* Read data from EEPROM */
#ifdef CONFIG_ONBOARD_DISPLAY
  /* Read this first because it affects device_hw_address() */
  if (size > 31)
    menu_system_enabled = eeprom_read_byte(&storedconfig.menu_system_enabled);
#endif
  tmp = eeprom_read_byte(&storedconfig.global_flags);
  globalflags &= (uint8_t)~(POSTMATCH | FASTFORMAT |
                            EXTENSION_HIDING | D64_WITH_HIDDEN);
  globalflags |= tmp & (POSTMATCH | FASTFORMAT |
                        EXTENSION_HIDING | D64_WITH_HIDDEN);

#ifdef CONFIG_ONBOARD_DISPLAY
  {
    uint8_t current_hw_addr = device_hw_address();

    if (current_hw_addr != eeprom_read_byte(&storedconfig.hardaddress)) {
      device_address = current_hw_addr;
      rewrite_required = true;
    } else {
      device_address = eeprom_read_byte(&storedconfig.address);
    }
    /* Reject an implausible address, valid are 4..30 */
    if (device_address < 4 || device_address > 30) {
      device_address = current_hw_addr;
      rewrite_required = true;
    }
  }
#else
  if (eeprom_read_byte(&storedconfig.hardaddress) == device_hw_address()) {
    tmp = eeprom_read_byte(&storedconfig.address);
    /* Ignore an address outside the valid CBM range */
    if (tmp >= 4 && tmp <= 30)
      device_address = tmp;
  }
#endif

  tmp = eeprom_read_byte(&storedconfig.fileexts);
  file_extension_mode = tmp & 0x0f;
#ifndef CONFIG_XE5
  /* A mode stored by a build with extension mode 5 is not available */
  if (file_extension_mode > 4)
    file_extension_mode = 1;
#endif
  file_extension_lock = (tmp >> 7) & 1;
  if (tmp & 0x40)
    globalflags |= CHECK_TYPE;
  else
    globalflags &= (uint8_t)~CHECK_TYPE;

#ifdef NEED_DISKMUX
  if (size > 9) {
    uint32_t tmpconfig;
    tmpconfig = eeprom_read_word(&storedconfig.drvconfig0);
    tmpconfig |= (uint32_t)eeprom_read_word(&storedconfig.drvconfig1) << 16;
    set_drive_config(tmpconfig);
  }

  /* sanity check.  If the user has truly turned off all drives, turn the
   * defaults back on
   */
  if(drive_config == 0xffffffff)
    set_drive_config(get_default_driveconfig());
#endif

  if (size > 13)
    image_as_dir = eeprom_read_byte(&storedconfig.imagedirs);

  if (size > 29)
    eeprom_read_block(rom_filename, &storedconfig.romname, ROM_NAME_LENGTH);

#ifdef CONFIG_ONBOARD_DISPLAY
  if (load_active_bus && size > 30)
    active_bus = eeprom_read_byte(&storedconfig.active_bus);
  /* Only IEC and IEEE488 are valid, anything else is a stale EEPROM */
  if (active_bus != IEC && active_bus != IEEE488) {
    active_bus = IEEE488;
    rewrite_required = true;
  }

  if (size > 32) {
    lcd_contrast = eeprom_read_byte(&storedconfig.lcd_contrast);
    lcd_brightness = eeprom_read_byte(&storedconfig.lcd_brightness);
    lcd_set_contrast(lcd_contrast);
    lcd_set_brightness(lcd_brightness);
  }
#endif

#ifdef CONFIG_LCD_WS0010
  if (size > 30)
    oled_dim_time = eeprom_read_byte(&storedconfig.oled_dim);
#endif

  /* Prevent problems due to accidental writes */
  eeprom_safety();

#ifdef CONFIG_ONBOARD_DISPLAY
  if (rewrite_required)
    write_configuration();
#endif
}

/**
 * write_configuration - stores configuration data to EEPROM
 *
 * This function stores the current configuration values to the EEPROM.
 */
void write_configuration(void) {
  uint_fast16_t i;
  uint8_t checksum;

  /* Write configuration to EEPROM */
  eeprom_write_word(&storedconfig.structsize, sizeof(storedconfig));
  eeprom_write_byte(&storedconfig.global_flags,
                    globalflags & (POSTMATCH | FASTFORMAT |
                                   EXTENSION_HIDING | D64_WITH_HIDDEN));
  eeprom_write_byte(&storedconfig.address, device_address);
  eeprom_write_byte(&storedconfig.hardaddress, device_hw_address());
  eeprom_write_byte(&storedconfig.fileexts,
                    file_extension_mode | (uint8_t)(file_extension_lock << 7) |
                    ((globalflags & CHECK_TYPE) ? 0x40 : 0));
#ifdef NEED_DISKMUX
  eeprom_write_word(&storedconfig.drvconfig0, drive_config);
  eeprom_write_word(&storedconfig.drvconfig1, drive_config >> 16);
#endif
  eeprom_write_byte(&storedconfig.imagedirs, image_as_dir);
  memset(rom_filename+ustrlen(rom_filename), 0, sizeof(rom_filename)-ustrlen(rom_filename));
  eeprom_write_block(rom_filename, &storedconfig.romname, ROM_NAME_LENGTH);
#ifdef CONFIG_ONBOARD_DISPLAY
  eeprom_write_byte(&storedconfig.active_bus, active_bus);
  eeprom_write_byte(&storedconfig.menu_system_enabled, menu_system_enabled);
  eeprom_write_byte(&storedconfig.lcd_contrast, lcd_contrast);
  eeprom_write_byte(&storedconfig.lcd_brightness, lcd_brightness);
#endif
#ifdef CONFIG_LCD_WS0010
  eeprom_write_byte(&storedconfig.oled_dim, oled_dim_time);
#endif

  /* Calculate checksum over EEPROM contents */
  checksum = 0;
  for (i=2;i<sizeof(storedconfig);i++)
    checksum += eeprom_read_byte((uint8_t *) i);

  /* Store checksum to EEPROM */
  eeprom_write_byte(&storedconfig.checksum, checksum);

  /* Prevent problems due to accidental writes */
  eeprom_safety();
}

