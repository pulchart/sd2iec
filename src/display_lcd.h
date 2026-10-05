/*
   SD2IEC LCD - SD/MMC to Commodore IEC bus controller with LCD support.
   Created 2008,2009 by Sascha Bader <sbader01@hotmail.com>

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

   display_lcd.c: lcd display routines and data
*/

#ifndef DISPLAY_LCD
#define DISPLAY_LCD

#include <stdint.h>
#include "config.h"

#ifdef CONFIG_LCD_DISPLAY

#include "bus.h"
#include "lcd.h"

#define MAXLINELENGHT 20

#define DS_INIT()     lcd_init(LCD_DISP_ON)
#define DS_BOOT()     lcd_boot()
#define DS_READY()    lcd_ready(device_address)
#define DS_ERROR(A)   lcd_error(A)
#define DS_LOAD(A)    lcd_show_name(1, 'L', (const char *)(A))
#define DS_SAVE(A)    lcd_show_name(1, 'S', (const char *)(A))
#define DS_CD(A)      lcd_path((const char *)(A))
#define DS_TICK()     lcd_scroll_poll()

void lcd_boot(void);
void lcd_clrline(int line);
void lcd_ready(uint8_t device);
void lcd_error(const uint8_t *msg);
void lcd_path(const char *path);
void lcd_show_name(uint8_t line, char tag, const char *name);
void lcd_scroll_poll(void);
uint8_t lcd_xcommand(uint8_t *cmd);

#else /* CONFIG_LCD_DISPLAY */

#define DS_INIT()     do { } while (0)
#define DS_BOOT()     do { } while (0)
#define DS_READY()    do { } while (0)
#define DS_ERROR(A)   do { } while (0)
#define DS_LOAD(A)    do { } while (0)
#define DS_SAVE(A)    do { } while (0)
#define DS_CD(A)      do { } while (0)
#define DS_TICK()     do { } while (0)

#endif /* CONFIG_LCD_DISPLAY */

#endif
