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

   display_lcd.h: lcd display routines and data
*/

#include "display_lcd.h"
#include <avr/pgmspace.h>

#include "iec-bus.h"
#include "parser.h"
#include "timer.h"
#include "uart.h"
#include <util/delay.h>
#include <string.h>

static uint8_t lcdcontrast;  // andi6510: LCD contrast setting

#define SCROLL_PREFIX 2
#define SCROLL_WIDTH  (LCD_DISP_LENGTH - SCROLL_PREFIX)
#define SCROLL_MAX    32
#define SCROLL_GAP    3
#define SCROLL_START  MS_TO_TICKS(1000)
#define SCROLL_STEP   MS_TO_TICKS(500)

static struct {
	uint8_t len;
	uint8_t off;
	uint8_t pos;
	tick_t  next;
	char    txt[SCROLL_MAX + 1];
} scroll[2];


static const char  mychars[64] PROGMEM = {
			0x00, 0x01, 0x07, 0x0F, 0x0F, 0x1E, 0x1C, 0x1C,
			 0x1F, 0x1F, 0x1F, 0x11, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1E, 0x1C, 0x00,
			 0x1C, 0x1E, 0x0F, 0x0F, 0x07, 0x01, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x11, 0x1F, 0x1F, 0x1F, 0x00,
			 0x1C, 0x1E, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x05, 0x07, 0x02, 0x00, 0x00,
			0x00, 0x1B, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00
	 };

#ifdef EEE
static const char eee1[16] PROGMEM  = {
 0x20, 0x06, 0x07, 0x06, 0x20, 0x20, 0x08, 0x01, 0x02, 0x06, 0x07, 0x06, 0x20, 0x07, 0x20, 0x20
};

static const char eee2[16] PROGMEM  = {
 0x20, 0x06, 0x20, 0x07, 0x06, 0x20, 0x03, 0x04, 0x05, 0x20, 0x06, 0x07, 0x06, 0x20, 0x06, 0x20
};
#endif

void lcd_clrline(int line)
{
	if (line < 2)
		scroll[line].len = 0;

	if (lcd_controller_type() != 0)
	{
		lcd_gotoxy(0,line);

		for (int i=0; i < MAXLINELENGHT; i++)
		{
    		lcd_putc(0x20);
		}
		lcd_gotoxy(0,line);
	}
}


static void lcd_cmdseq(char * cmdseq)
{
	if (lcd_controller_type() != 0)
	{
		lcd_command(cmdseq[0]);
		lcd_puts(cmdseq+1);
	}
}

static void lcd_setCustomChars(void)
{
	char * progmem_s;

	if (lcd_controller_type() != 0)
	{
		lcd_command(0x40);  // write to CGRAM
		progmem_s  = (char *) mychars;
		for (int i=0;i<64;i++)
		{
		  _delay_us(64);
		  lcd_data( pgm_read_byte(progmem_s++) );
		}
	}
}

static void lcd_eee(void)
{
#ifdef EEE
	char * progmem_s;
	if (lcd_controller_type() != 0)
	{
		lcd_setCustomChars();
		lcd_clrscr();
		lcd_gotoxy(0,0);
		progmem_s = (char *) eee1;
		for (int j=0;j<16;j++)
		{
		  lcd_putc( pgm_read_byte(progmem_s++) );
		}
		lcd_gotoxy(0,1);
		progmem_s = (char *) eee2;
		for (int j=0;j<16;j++)
		{
		  lcd_putc( pgm_read_byte(progmem_s++) );
		}
	}
#endif
}

static void lcd_logo(void)
{
	if (lcd_controller_type() != 0)
	{
		lcd_setCustomChars();
		lcd_clrscr();
		lcd_putc(0); // andi6510 - initally the character 8 was set but on my DOG-M display this was not the correct one...
		lcd_putc(1);
		lcd_putc(2);
		lcd_gotoxy(0,1);
		lcd_putc(3);
		lcd_putc(4);
		lcd_putc(5);
		lcd_gotoxy(4,0);
		lcd_puts_p(PSTR("Commodore"));
		lcd_gotoxy(4,1);
		lcd_puts_p(PSTR("never dies!"));
	}
}

void lcd_ready(uint8_t dev_addr)
{
	if (lcd_controller_type() != 0)
	{
		lcd_clrline(1);
		lcd_puts_p(PSTR("READY:"));
		lcd_putc('0' + dev_addr / 10);
		lcd_putc('0' + dev_addr % 10);
	}
}

void lcd_show_name(uint8_t line, char tag, const char *name)
{
	uint8_t len = strlen(name);
	uint8_t i;

	if (lcd_controller_type() == 0)
		return;

	lcd_clrline(line);
	lcd_putc(tag);
	lcd_putc(':');
	for (i = 0; i < len && i < SCROLL_WIDTH; i++)
		lcd_putc(name[i]);

	if (len > SCROLL_WIDTH && line < 2) {
		if (len > SCROLL_MAX)
			len = SCROLL_MAX;
		memcpy(scroll[line].txt, name, len);
		scroll[line].len  = len;
		scroll[line].off  = 0;
		scroll[line].pos  = SCROLL_WIDTH;
		scroll[line].next = getticks() + SCROLL_START;
	}
}

void lcd_path(const char *fs_path)
{
	lcd_show_name(0, 'D', fs_path);
}

/* Called from the bus idle loop */
void lcd_scroll_poll(void)
{
	uint8_t l;

	for (l = 0; l < 2; l++) {
		uint8_t len = scroll[l].len;
		uint8_t idx;

		if (!len)
			continue;

		if (scroll[l].pos < SCROLL_WIDTH) {
			lcd_gotoxy(SCROLL_PREFIX + scroll[l].pos, l);
			while (scroll[l].pos < SCROLL_WIDTH && IEC_ATN) {
				idx = scroll[l].off + scroll[l].pos;
				idx %= len + SCROLL_GAP;
				lcd_putc(idx < len ? scroll[l].txt[idx] : ' ');
				scroll[l].pos++;
			}
		} else if (time_after(getticks(), scroll[l].next)) {
			scroll[l].off = (scroll[l].off + 1) % (len + SCROLL_GAP);
			scroll[l].pos = 0;
			scroll[l].next = getticks() + SCROLL_STEP;
		}
	}
}

static void lcd_contrast(uint8_t contrast)
{
	// andi6510: in case we have a st7036 LCD set contrast value by software
	if (lcd_controller_type() == 7)
	{
		lcdcontrast = contrast;
		lcd_command(0x29); // activate instruction table 1
		lcd_command(0x50 | ((0x30 & contrast) >> 4)); // icon off, booster off, contrast XX....
		lcd_command(0x70 |  (0x0F & contrast)      ); // contrast ...XXXX
		lcd_command(0x28); // activate instruction table 0
	}
}
static void lcd_title(void)
{
	lcd_clrline(0);
	lcd_puts_p(PSTR("SD2IEC " LCDVERSION));
}

void lcd_boot(void)
{
	uart_puts_P(PSTR("\r\nLCD: "));
	switch (lcd_controller_type()) {
	case 4:
		uart_puts_P(PSTR("HD44780"));
		break;
	case 7:
		uart_puts_P(PSTR("ST7036"));
		break;
	default:
		uart_puts_P(PSTR("none"));
		break;
	}
	uart_putcrlf();

	lcd_clrscr();
	lcd_logo();
	_delay_ms(1000);
	lcd_title();
	lcd_ready(device_address);
}

void lcd_error(const uint8_t *msg)
{
	lcd_clrline(1);
	lcd_puts_p(PSTR("E:"));
	lcd_puts((const char *)msg);
}

/* X-commands XT, XA, XG, XX, XC; returns 1 on syntax error */
uint8_t lcd_xcommand(uint8_t *cmd)
{
	uint8_t *str;
	uint16_t num;

	switch (cmd[1]) {
	case 'T':
		str = cmd + 3;
		if (cmd[2] == '1' || cmd[2] == '2') {
			lcd_clrline(cmd[2] - '1');
			lcd_puts((char *)str);
		} else if (cmd[2] == 'C') {
			lcd_cmdseq((char *)str);
		}
		break;

	case 'A':
		lcd_title();
		lcd_clrline(1);
		lcd_puts_p(PSTR("2009 by S. Bader"));
		break;

	case 'G':
		lcd_clrline(0);
		lcd_puts_p(PSTR("Credits 2 Unseen"));
		lcd_clrline(1);
		lcd_puts_p(PSTR(" and Shadowolf!"));
		break;

	case 'X':
		lcd_eee();
		break;

	case 'C':
		str = cmd + 2;
		num = parse_number(&str);
		if (num >= 64)
			return 1;
		lcd_clrline(1);
		lcd_puts_p(PSTR("Contrast: "));
		lcd_putc('0' + num / 10);
		lcd_putc('0' + num % 10);
		lcd_contrast(num);
		break;
	}
	return 0;
}
