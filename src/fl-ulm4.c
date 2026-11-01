/* sd2iec - SD/MMC to Commodore serial bus interface/controller
   Copyright (C) 2007-2026  Ingo Korb <ingo@akana.de>
   ULoad Model 4 support:
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


   fl-ulm4.c: High level handling of ULoad Model 4 (Ultima IV Remastered)

*/

#include <stdbool.h>
#include <string.h>
#include "config.h"

#ifdef CONFIG_LOADER_ULOAD4

#include "buffers.h"
#include "d64ops.h"
#include "display_lcd.h"
#include "errormsg.h"
#include "fastloader-ll.h"
#include "fileops.h"
#include "iec-bus.h"
#include "iec.h"
#include "led.h"
#include "parser.h"
#include "wrapops.h"
#include "fastloader.h"

#define U4_NAME_LEN   16    /* bytes compared with a directory entry */
#define U4_MAX_CHAIN  4096  /* sectors per file, more than any image has */

#define U4_REPLY_OK       0x00
#define U4_REPLY_UNKNOWN  0x80
#define U4_REPLY_NOTFOUND 0x81
#define U4_REPLY_ERROR    0xff

/* State the drive code keeps between commands */
typedef struct {
  buffer_t *buf;     /* last sector read */
  uint8_t   track;
  uint8_t   sector;
  uint8_t   rs_len;  /* bytes sent for a sector, 0 = 256 */
  uint8_t   type;    /* image type for the current command */
} u4_state_t;


/* ---- Transfer, the bit level is the ULoad Model 3 protocol ---- */

/* Send a byte to the C64, true if aborted by ATN or a key */
static bool u4_put(uint8_t b) {
  set_data(0);
  while (IEC_CLOCK) {
    if (!IEC_ATN || check_keys()) {
      set_data(1);
      return true;
    }
  }
  uload3_send_byte(b);
  return !IEC_ATN;
}

/* Receive a byte from the C64, <0 if aborted by ATN or a key */
static int16_t u4_get(void) {
  set_clock(0);
  while (IEC_DATA) {
    if (!IEC_ATN || check_keys()) {
      set_clock(1);
      return -1;
    }
    /* The C64 waits for CLK to be released, so a slow display update is safe */
    DS_TICK();
  }
  return uload3_get_byte();
}

static bool u4_recv(uint8_t *value) {
  int16_t v = u4_get();

  if (v < 0)
    return true;

  *value = v;
  return false;
}


/* ---- Drive state helpers ---- */

/* Image type if the loader can serve it, D64_TYPE_NONE otherwise */
static uint8_t u4_image_type(void) {
  uint8_t type;

  if (partition[current_part].fop != &d64ops)
    return D64_TYPE_NONE;

  type = partition[current_part].imagetype & D64_TYPE_MASK;
  if (type == D64_TYPE_D41 || type == D64_TYPE_D71 || type == D64_TYPE_D81)
    return type;

  return D64_TYPE_NONE;
}

/* Read a sector into the buffer, true on error */
static bool u4_read(u4_state_t *s, uint8_t track, uint8_t sector) {
  if (s->type == D64_TYPE_NONE)
    return true;

  current_error = 0;
  read_sector(s->buf, current_part, track, sector);
  return current_error != 0;
}

/* Write the buffer to track/sector, true on error */
static bool u4_write(u4_state_t *s) {
  current_error = 0;
  write_sector(s->buf, current_part, s->track, s->sector);
  return current_error != 0;
}

/* Follow the links of the last sector read, true at the end of the chain */
static bool u4_nextts(u4_state_t *s) {
  if (s->buf->data[0] == 0)
    return true;

  s->track  = s->buf->data[0];
  s->sector = s->buf->data[1];
  return false;
}

/* Directory start of the image type, false if there is none */
static bool u4_dirstart(u4_state_t *s) {
  if (s->type == D64_TYPE_D81) {
    s->track  = 40;
    s->sector = 3;
  } else if (s->type != D64_TYPE_NONE) {
    s->track  = 18;
    s->sector = 1;
  } else
    return false;

  return true;
}

/* Find a closed unlocked PRG with exactly this name, leaves its start in track/sector */
static bool u4_find(u4_state_t *s, const uint8_t *name) {
  uint8_t  sectors = 0;
  uint8_t  entry;
  uint8_t *ptr;

  if (!u4_dirstart(s))
    return false;

  do {
    if (u4_read(s, s->track, s->sector))
      return false;

    ptr = s->buf->data + 2;
    for (entry = 0; entry < 8; entry++) {
      if (ptr[0] == 0x82) {
        s->track  = ptr[1];
        s->sector = ptr[2];
        if (memcmp(name, ptr + 3, U4_NAME_LEN) == 0)
          return true;
      }
      ptr += 32;
    }

    if (u4_nextts(s))
      return false;
  } while (++sectors != 0);

  return false;
}


/* ---- Commands ---- */

/* Commands 1 and 2: load or save-and-replace a file by name, true if aborted */
static bool u4_file(u4_state_t *s, bool save) {
  uint8_t  name[U4_NAME_LEN];
  uint8_t  len, i, n, b;
  uint16_t sectors = 0;
  bool     found;

  memset(name, 0xa0, sizeof(name));
  if (u4_recv(&len))
    return true;

  i = 0;
  do {
    if (u4_recv(&b))
      return true;
    if (i < sizeof(name))
      name[i] = b;
    i++;
  } while (i != len);

  if (name[0] == '$')
    /* the directory chain, not writable */
    found = !save && u4_dirstart(s);
  else
    found = u4_find(s, name);

  if (found && save && !u4_read(s, s->track, s->sector))
    /* answer a read-only image before any data is exchanged */
    found = !u4_write(s);

  if (!found)
    return u4_put(U4_REPLY_NOTFOUND);

  if (u4_put(U4_REPLY_OK))
    return true;

  for (;;) {
    if (u4_read(s, s->track, s->sector))
      return u4_put(U4_REPLY_ERROR);

    /* bytes in this sector, 0 ends the stream */
    n = s->buf->data[0] ? 254 : s->buf->data[1] - 1;
    if (n > 254)
      n = 0;

    if (u4_put(n))
      return true;
    if (n == 0)
      return false;

    for (i = 0; i < n; i++) {
      if (save) {
        if (u4_recv(&b))
          return true;
        s->buf->data[2 + i] = b;
      } else if (u4_put(s->buf->data[2 + i]))
        return true;
    }

    if (save && u4_write(s))
      return u4_put(U4_REPLY_ERROR);

    if (u4_nextts(s))
      break;

    if (++sectors == U4_MAX_CHAIN)
      return u4_put(U4_REPLY_ERROR);
  }

  return u4_put(U4_REPLY_OK);
}

/* Commands 3 and 4: send the sector at track/sector, true if aborted */
static bool u4_sector(u4_state_t *s) {
  uint8_t i = 0;

  if (u4_read(s, s->track, s->sector))
    return u4_put(U4_REPLY_ERROR);

  if (u4_put(U4_REPLY_OK))
    return true;

  do {
    if (u4_put(s->buf->data[i]))
      return true;
    i++;
  } while (i != s->rs_len);

  return false;
}

/* Command 5: links of all sectors of a track, highest sector first, true if aborted */
static bool u4_links(u4_state_t *s) {
  uint8_t track, x, n, link[2];

  if (u4_recv(&track))
    return true;

  s->track = track;
  n = (s->type == D64_TYPE_D81) ? 39 : 20;

  /* sector 0 must exist, it also validates the track */
  if (u4_read(s, track, 0))
    return u4_put(U4_REPLY_ERROR);

  if (u4_put(n))
    return true;

  for (x = n; ; x--) {
    link[0] = 0;
    link[1] = 0;

    /* sectors beyond the end of the track and unreadable ones give zeros */
    if (x < d64_sectors_per_track(current_part, track) && !u4_read(s, track, x)) {
      link[0] = s->buf->data[0];
      link[1] = s->buf->data[1];
    }

    if (u4_put(link[0]) || u4_put(link[1]))
      return true;

    if (x == 0)
      break;
  }

  return false;
}


bool load_uload4(UNUSED_PARAMETER) {
  u4_state_t s;
  int16_t    cmd;
  bool       quit = false;

  if (u4_image_type() == D64_TYPE_NONE) {
    set_error(ERROR_IMAGE_INVALID);
    return true;
  }

  s.buf = alloc_system_buffer();
  if (s.buf == NULL)
    return true;

  memset(s.buf->data, 0, 256);
  s.track  = 0;
  s.sector = 0;
  s.rs_len = 0xff;

  set_clock(1);
  set_data(1);

  while (!quit) {
    set_busy_led(0);
    cmd = u4_get();
    if (cmd < 0)
      break;

    set_busy_led(1);
    s.type = u4_image_type();

    switch (cmd) {
    case 1:
      quit = u4_file(&s, false);
      break;

    case 2:
      set_dirty_led(1);
      quit = u4_file(&s, true);
      set_dirty_led(0);
      break;

    case 3:
      quit = u4_recv(&s.track) || u4_recv(&s.sector) || u4_recv(&s.rs_len) ||
              u4_sector(&s);
      break;

    case 4:
      u4_nextts(&s);
      quit = u4_sector(&s);
      break;

    case 5:
      quit = u4_links(&s);
      break;

    default:
      quit = u4_put(U4_REPLY_UNKNOWN);
      break;
    }
  }

  set_busy_led(0);
  set_dirty_led(0);
  set_clock(1);
  set_data(1);
  free_buffer(s.buf);

  /* do not leave a failed sector read in the status */
  if (current_error != 0)
    set_error(ERROR_OK);

  return true;
}

#endif /* CONFIG_LOADER_ULOAD4 */
