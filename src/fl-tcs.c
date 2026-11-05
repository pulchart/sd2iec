/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2007-2026  Ingo Korb <ingo@akana.de>
   TCS fast loader of Elite 64 support:
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


   fl-tcs.c: High level handling of the TCS fast loader of Elite 64 (C64)

*/

#include <stdbool.h>
#include <string.h>
#include "config.h"

#ifdef CONFIG_LOADER_TCS

#include "buffers.h"
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

#define TCS_TIMEOUT 200 /* units of 10 ms the C64 may take to pull ATN low */


/* Wait until the C64 pulls ATN low, true if it did not or a key was pressed */
static bool tcs_wait(void) {
  uint8_t n = 0;

  while (IEC_ATN) {
    start_timeout(10000);
    while (IEC_ATN && !has_timed_out()) ;

    if (IEC_ATN && (++n == TCS_TIMEOUT || check_keys()))
      return true;
  }

  return false;
}

/* Send a byte, true if aborted. The C64 waits until DATA is released */
static bool tcs_put(uint8_t b) {
  if (tcs_wait())
    return true;

  tcs_send_byte(b);
  return false;
}


/* The file is opened as usual, the C64 receives its sectors from the first */
/* byte of the link on. A first byte of 0 marks the last sector, followed   */
/* by the index of the last byte + 1. $ff signals an error                  */
static void tcs_load_file(void) {
  buffer_t *buf = find_buffer(0);
  uint16_t  i;

  if (buf == NULL)
    return;

  for (;;) {
    if (buf->sendeoi) {
      if (tcs_put(0) || tcs_put(buf->lastused + 1))
        return;
    } else {
      /* the link of a sector that is not the last one is not used */
      if (tcs_put(1) || tcs_put(0))
        return;
    }

    for (i = 2; i <= buf->lastused; i++)
      if (tcs_put(buf->data[i]))
        return;

    if (buf->sendeoi)
      return;

    if (buf->refill(buf)) {
      tcs_put(0xff);
      return;
    }
  }
}


bool load_tcs(uint8_t unused) {
  (void)unused;

  uart_flush(); /* pending output would disturb the timing */

  set_atn_irq(0);

  /* busy until the first byte is ready */
  set_data(0);
  set_clock(0);

  set_busy_led(1);

  tcs_load_file();

  set_busy_led(0);

  set_clock(1);
  set_data(1);

  return true;
}

#endif /* CONFIG_LOADER_TCS */
