#ifndef LCD_H
#define LCD_H

/*
   HD44780U LCD library, 4-bit IO port mode, two lines.
   Author: Peter Fleury <pfleury@gmx.ch>  http://jump.to/fleury
   Changed for SD2IEC LCD by Sascha Bader <sbader01@hotmail.com>:
   hardware variant selection. ST7036 support and auto detection by andi6510.
*/

#include <inttypes.h>
#include <avr/pgmspace.h>
#include "config.h"

#define LCD_DISP_LENGTH    16     /* visible characters per line */
#define LCD_START_LINE1  0x00     /* DDRAM address of first char of line 1 */
#define LCD_START_LINE2  0x40     /* DDRAM address of first char of line 2 */

/* LCD pins per hardware variant: data lines DB4-DB7 and RS, RW, E */

/* Lars Pontoppidan variant (larsp) */
#if CONFIG_HARDWARE_VARIANT==3

#define LCD_PORT         PORTC        /**< port for the LCD lines   */
#define LCD_DATA0_PORT   LCD_PORT     /**< port for 4bit data bit 0 */
#define LCD_DATA1_PORT   LCD_PORT     /**< port for 4bit data bit 1 */
#define LCD_DATA2_PORT   LCD_PORT     /**< port for 4bit data bit 2 */
#define LCD_DATA3_PORT   LCD_PORT     /**< port for 4bit data bit 3 */
#define LCD_DATA0_PIN    4            /**< pin for 4bit data bit 0  */
#define LCD_DATA1_PIN    5            /**< pin for 4bit data bit 1  */
#define LCD_DATA2_PIN    6            /**< pin for 4bit data bit 2  */
#define LCD_DATA3_PIN    7            /**< pin for 4bit data bit 3  */
#define LCD_RS_PORT      PORTB        /**< port for RS line         */
#define LCD_RS_PIN       0            /**< pin  for RS line         */
#define LCD_RW_PORT      PORTB        /**< port for RW line         */
#define LCD_RW_PIN       1            /**< pin  for RW line         */
#define LCD_E_PORT       PORTB        /**< port for Enable line     */
#define LCD_E_PIN        2            /**< pin  for Enable line     */

#endif

/* Shadowolf 1 (sw1) */
#if CONFIG_HARDWARE_VARIANT==2

#define LCD_PORT         PORTA        /**< port for the LCD lines   */
#define LCD_DATA0_PORT   LCD_PORT     /**< port for 4bit data bit 0 */
#define LCD_DATA1_PORT   LCD_PORT     /**< port for 4bit data bit 1 */
#define LCD_DATA2_PORT   LCD_PORT     /**< port for 4bit data bit 2 */
#define LCD_DATA3_PORT   LCD_PORT     /**< port for 4bit data bit 3 */
#define LCD_DATA0_PIN    4            /**< pin for 4bit data bit 0  */
#define LCD_DATA1_PIN    5            /**< pin for 4bit data bit 1  */
#define LCD_DATA2_PIN    6            /**< pin for 4bit data bit 2  */
#define LCD_DATA3_PIN    7            /**< pin for 4bit data bit 3  */
#define LCD_RS_PORT      PORTB        /**< port for RS line         */
#define LCD_RS_PIN       0            /**< pin  for RS line         */
#define LCD_RW_PORT      PORTB        /**< port for RW line         */
#define LCD_RW_PIN       1            /**< pin  for RW line         */
#define LCD_E_PORT       PORTB        /**< port for Enable line     */
#define LCD_E_PIN        2            /**< pin  for Enable line     */

#endif

/* Shadowolf 2 variant (sw2) */
/* added by SBa */
#if CONFIG_HARDWARE_VARIANT==5

#define LCD_PORT         PORTB        /**< port for the LCD lines   */
#define LCD_DATA0_PORT   PORTC        /**< port for 4bit data bit 0 */
#define LCD_DATA1_PORT   PORTC        /**< port for 4bit data bit 1 */
#define LCD_DATA2_PORT   PORTC        /**< port for 4bit data bit 2 */
#define LCD_DATA3_PORT   PORTC        /**< port for 4bit data bit 3 */
#define LCD_DATA0_PIN    4            /**< pin for 4bit data bit 0  */
#define LCD_DATA1_PIN    5            /**< pin for 4bit data bit 1  */
#define LCD_DATA2_PIN    6            /**< pin for 4bit data bit 2  */
#define LCD_DATA3_PIN    7            /**< pin for 4bit data bit 3  */
#define LCD_RS_PORT      PORTB        /**< port for RS line         */
#define LCD_RS_PIN       0            /**< pin  for RS line         */
#define LCD_RW_PORT      PORTB        /**< port for RW line         */
#define LCD_RW_PIN       1            /**< pin  for RW line         */
#define LCD_E_PORT       PORTB        /**< port for Enable line     */
#define LCD_E_PIN        3            /**< pin  for Enable line     */

#endif

/*------------------------- End of changes by SBa ----------------------- */

/* evo2 variant */
/* added by 16x8 */

#if CONFIG_HARDWARE_VARIANT==11

#define LCD_PORT         PORTC        /**< port for the LCD lines   */
#define LCD_DATA0_PORT   LCD_PORT     /**< port for 4bit data bit 0 */
#define LCD_DATA1_PORT   LCD_PORT     /**< port for 4bit data bit 1 */
#define LCD_DATA2_PORT   LCD_PORT     /**< port for 4bit data bit 2 */
#define LCD_DATA3_PORT   LCD_PORT     /**< port for 4bit data bit 3 */
#define LCD_DATA0_PIN    4            /**< pin for 4bit data bit 0  */
#define LCD_DATA1_PIN    5            /**< pin for 4bit data bit 1  */
#define LCD_DATA2_PIN    6            /**< pin for 4bit data bit 2  */
#define LCD_DATA3_PIN    7            /**< pin for 4bit data bit 3  */
#define LCD_RS_PORT      PORTB        /**< port for RS line         */
#define LCD_RS_PIN       0            /**< pin  for RS line         */
#define LCD_RW_PORT      PORTB        /**< port for RW line         */
#define LCD_RW_PIN       1            /**< pin  for RW line         */
#define LCD_E_PORT       PORTB        /**< port for Enable line     */
#define LCD_E_PIN        2            /**< pin  for Enable line     */

#endif

/* HD44780U instruction bits and codes */
#define LCD_CLR               0      /* DB0: clear display */
#define LCD_ENTRY_MODE        2      /* DB2: set entry mode */
#define LCD_ENTRY_INC         1      /* DB1: 1=increment, 0=decrement */
#define LCD_FUNCTION          5      /* DB5: function set */
#define LCD_FUNCTION_8BIT     4      /* DB4: 8 bit mode, 0=4 bit mode */
#define LCD_DDRAM             7      /* DB7: set DD RAM address */
#define LCD_BUSY              7      /* DB7: LCD is busy */

#define LCD_DISP_OFF             0x08   /* display off */
#define LCD_DISP_ON              0x0C   /* display on, cursor off */
#define LCD_DISP_ON_CURSOR       0x0E   /* display on, cursor on */

#define LCD_FUNCTION_4BIT_1LINE  0x20   /* 4-bit interface, one line, 5x7 dots */
#define LCD_FUNCTION_4BIT_2LINES 0x28   /* 4-bit interface, two lines, 5x7 dots */

#define LCD_MODE_DEFAULT     ((1<<LCD_ENTRY_MODE) | (1<<LCD_ENTRY_INC))

void lcd_init(uint8_t dispAttr);
void lcd_clrscr(void);
void lcd_gotoxy(uint8_t x, uint8_t y);
void lcd_putc(char c);
void lcd_puts(const char *s);
void lcd_puts_p(const char *progmem_s);
void lcd_command(uint8_t cmd);
void lcd_data(uint8_t data);

/* 7: ST7036, 4: HD44780, 0: no display found */
uint8_t lcd_controller_type(void);

#endif
