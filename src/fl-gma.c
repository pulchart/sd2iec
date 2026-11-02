/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2007-2026  Ingo Korb <ingo@akana.de>
   GMA fast loader of Elite support:
   Copyright (C) 2026  Jaroslav Pulchart <jaroslav.pulchart@gmail.com>

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


   fl-gma.c: High level handling of the GMA fast loader of Elite (C64)

*/

#include <stdbool.h>
#include <string.h>
#include "config.h"

#ifdef CONFIG_LOADER_GMA

#include "buffers.h"
#include "d64ops.h"
#include "errormsg.h"
#include "fastloader-ll.h"
#include "fileops.h"
#include "iec-bus.h"
#include "iec.h"
#include "led.h"
#include "parser.h"
#include "timer.h"
#include "uart.h"
#include "wrapops.h"
#include "fastloader.h"

#define GMA_MAX_CHAIN 4096  /* sectors per file, more than any image has */
#define GMA_TIMEOUT   200   /* units of 10 ms the C64 may take to pull ATN low */

/* Track and sector of the file in the drive code of the GMA86 PAL variant */
uint8_t gma_track;
uint8_t gma_sector;


/* Wait until the C64 pulls ATN low, true if it did not or a key was pressed */
static bool gma_wait(void) {
  uint8_t n = 0;

  while (IEC_ATN) {
    start_timeout(10000);
    while (IEC_ATN && !has_timed_out()) ;

    if (IEC_ATN && (++n == GMA_TIMEOUT || check_keys()))
      return true;
  }

  return false;
}

/* Send a byte, true if aborted. The C64 waits until DATA is released */
static bool gma_put(uint8_t b, bool ntsc) {
  if (gma_wait())
    return true;

  if (ntsc)
    gma_send_byte_ntsc(b);
  else
    gma_send_byte_pal(b);

  return false;
}


/* GMA86 PAL: the C64 sends track and sector of the file and receives its   */
/* data, 1 is sent twice. The end is 1 followed by 2, the load address is   */
/* the first data of the file                                               */
static void gma_load_sectors(void) {
  buffer_t *buf;
  uint8_t   track  = gma_track;
  uint8_t   sector = gma_sector;
  uint16_t  chain  = 0;
  uint16_t  i, end;

  if (partition[current_part].fop != &d64ops) {
    set_error(ERROR_IMAGE_INVALID);
    return;
  }

  buf = alloc_system_buffer();
  if (buf == NULL)
    return;

  for (;;) {
    current_error = 0;
    read_sector(buf, current_part, track, sector);
    if (current_error != 0)
      break;

    end = buf->data[0] ? 255 : buf->data[1];

    for (i = 2; i <= end; i++) {
      if (buf->data[i] == 1 && gma_put(1, false))
        goto done;

      if (gma_put(buf->data[i], false))
        goto done;
    }

    if (buf->data[0] == 0) {
      if (gma_put(1, false) || gma_put(2, false))
        goto done;

      current_error = 0;
      break;
    }

    track  = buf->data[0];
    sector = buf->data[1];

    if (++chain == GMA_MAX_CHAIN)
      break;
  }

 done:
  free_buffer(buf);
}


/* GMA85 NTSC: the file is opened as usual, the C64 receives its sectors   */
/* from the first byte of the link on. A first byte of 0 marks the last    */
/* sector, followed by the index of the last byte + 1. $ff signals an error */
static void gma_load_file(void) {
  buffer_t *buf = find_buffer(0);
  uint16_t  i;

  if (buf == NULL)
    return;

  for (;;) {
    if (buf->sendeoi) {
      if (gma_put(0, true) || gma_put(buf->lastused + 1, true))
        return;
    } else {
      /* the link of a sector that is not the last one is not used */
      if (gma_put(1, true) || gma_put(0, true))
        return;
    }

    for (i = 2; i <= buf->lastused; i++)
      if (gma_put(buf->data[i], true))
        return;

    if (buf->sendeoi)
      return;

    if (buf->refill(buf)) {
      gma_put(0xff, true);
      return;
    }
  }
}


bool load_gma(uint8_t ntsc) {
  uart_flush(); /* pending output would disturb the timing */

  set_atn_irq(0);

  /* busy until the first byte is ready */
  set_data(0);
  set_clock(0);

  set_busy_led(1);

  if (ntsc)
    gma_load_file();
  else
    gma_load_sectors();

  set_busy_led(0);

  set_clock(1);
  set_data(1);

  return true;
}

#endif /* CONFIG_LOADER_GMA */
