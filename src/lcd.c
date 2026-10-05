/****************************************************************************
Title	:   HD44780U LCD library
 Author:    Peter Fleury <pfleury@gmx.ch>  http://jump.to/fleury
 File:	    $Id: lcd.c,v 1.14.2.1 2006/01/29 12:16:41 peter Exp $
 Software:  AVR-GCC 3.3
 Target:    any AVR device

 DESCRIPTION
       Basic routines for interfacing a HD44780U-based text lcd display

       Originally based on Volker Oth's lcd library,
       changed lcd_init(), added additional constants for lcd_command(),
       added 4-bit I/O mode, improved and optimized code.

       Only the 4-bit I/O port mode with two lines is kept.

 USAGE
       See the C include lcd.h file for a description of each function

 HISTORY
       Jan 2013: andi6510 added LCD auto detection functionality and support 
	             for ST7036 controllers.

*****************************************************************************/
#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "uart.h"
#include "lcd.h"

static uint8_t lcd_autodetect = 0;

/*
** constants/macros
*/
#define DDR(x) (*(&x - 1))      /* address of data direction register of port x */
#define PIN(x) (*(&x - 2))    /* address of input register of port x          */


#define lcd_e_delay()   __asm__ __volatile__( "rjmp 1f\n 1: rjmp 1f\n 1:" ); // delay 500ns (4 cycles)
#define lcd_e_high()    LCD_E_PORT  |=  _BV(LCD_E_PIN);
#define lcd_e_low()     LCD_E_PORT  &= ~_BV(LCD_E_PIN);
#define lcd_e_toggle()  toggle_e()
#define lcd_rw_high()   LCD_RW_PORT |=  _BV(LCD_RW_PIN)
#define lcd_rw_low()    LCD_RW_PORT &= ~_BV(LCD_RW_PIN)
#define lcd_rs_high()   LCD_RS_PORT |=  _BV(LCD_RS_PIN)
#define lcd_rs_low()    LCD_RS_PORT &= ~_BV(LCD_RS_PIN)

#define LCD_FUNCTION_DEFAULT    LCD_FUNCTION_4BIT_2LINES

/*
** function prototypes
*/
static void toggle_e(void);

/*
** local functions
*/



/*************************************************************************
 delay loop for small accurate delays: 16-bit counter, 4 cycles/loop
*************************************************************************/
static inline void _delayFourCycles(unsigned int __count)
{
    if ( __count == 0 )
        __asm__ __volatile__( "rjmp 1f\n 1:" );    // 2 cycles
    else
        __asm__ __volatile__ (
    	    "1: sbiw %0,1" "\n\t"
    	    "brne 1b"                              // 4 cycles/loop
    	    : "=w" (__count)
    	    : "0" (__count)
    	   );
}


/*************************************************************************
delay for a minimum of <us> microseconds
the number of loops is calculated at compile-time from MCU clock frequency
*************************************************************************/
#define delay(us)  _delay_us(us)


/* toggle Enable Pin to initiate write */
static void toggle_e(void)
{
    lcd_e_delay();      /* data setup time before E rises */
    lcd_e_high();
    lcd_e_delay();
    lcd_e_low();
}


/*************************************************************************
Low-level function to write byte to LCD controller
Input:    data   byte to write to LCD
          rs     1: write data
                 0: write instruction
Returns:  none
*************************************************************************/
static void lcd_write(uint8_t data,uint8_t rs)
{
    if (rs) {   /* write data        (RS=1, RW=0) */
       lcd_rs_high();
    } else {    /* write instruction (RS=0, RW=0) */
       lcd_rs_low();
    }
    lcd_rw_low();

    /* configure data pins as output */
    DDR(LCD_DATA0_PORT) |= _BV(LCD_DATA0_PIN);
    DDR(LCD_DATA1_PORT) |= _BV(LCD_DATA1_PIN);
    DDR(LCD_DATA2_PORT) |= _BV(LCD_DATA2_PIN);
    DDR(LCD_DATA3_PORT) |= _BV(LCD_DATA3_PIN);

    /* output high nibble first */
    LCD_DATA3_PORT &= ~_BV(LCD_DATA3_PIN);
    LCD_DATA2_PORT &= ~_BV(LCD_DATA2_PIN);
    LCD_DATA1_PORT &= ~_BV(LCD_DATA1_PIN);
    LCD_DATA0_PORT &= ~_BV(LCD_DATA0_PIN);
    if(data & 0x80) LCD_DATA3_PORT |= _BV(LCD_DATA3_PIN);
    if(data & 0x40) LCD_DATA2_PORT |= _BV(LCD_DATA2_PIN);
    if(data & 0x20) LCD_DATA1_PORT |= _BV(LCD_DATA1_PIN);
    if(data & 0x10) LCD_DATA0_PORT |= _BV(LCD_DATA0_PIN);
    lcd_e_toggle();

    /* output low nibble */
    LCD_DATA3_PORT &= ~_BV(LCD_DATA3_PIN);
    LCD_DATA2_PORT &= ~_BV(LCD_DATA2_PIN);
    LCD_DATA1_PORT &= ~_BV(LCD_DATA1_PIN);
    LCD_DATA0_PORT &= ~_BV(LCD_DATA0_PIN);
    if(data & 0x08) LCD_DATA3_PORT |= _BV(LCD_DATA3_PIN);
    if(data & 0x04) LCD_DATA2_PORT |= _BV(LCD_DATA2_PIN);
    if(data & 0x02) LCD_DATA1_PORT |= _BV(LCD_DATA1_PIN);
    if(data & 0x01) LCD_DATA0_PORT |= _BV(LCD_DATA0_PIN);
    lcd_e_toggle();

    /* all data pins high (inactive) */
    LCD_DATA0_PORT |= _BV(LCD_DATA0_PIN);
    LCD_DATA1_PORT |= _BV(LCD_DATA1_PIN);
    LCD_DATA2_PORT |= _BV(LCD_DATA2_PIN);
    LCD_DATA3_PORT |= _BV(LCD_DATA3_PIN);
}


/*************************************************************************
Low-level function to read byte from LCD controller
Input:    rs     1: read data
                 0: read busy flag / address counter
Returns:  byte read from LCD controller
*************************************************************************/
static uint8_t lcd_read(uint8_t rs)
{
    uint8_t data;

    if (rs)
        lcd_rs_high();                       /* RS=1: read data      */
    else
        lcd_rs_low();                        /* RS=0: read busy flag */
    lcd_rw_high();                           /* RW=1  read mode      */

    /* configure data pins as input */
    DDR(LCD_DATA0_PORT) &= ~_BV(LCD_DATA0_PIN);
    DDR(LCD_DATA1_PORT) &= ~_BV(LCD_DATA1_PIN);
    DDR(LCD_DATA2_PORT) &= ~_BV(LCD_DATA2_PIN);
    DDR(LCD_DATA3_PORT) &= ~_BV(LCD_DATA3_PIN);

    /* read high nibble first */
    lcd_e_high();
    lcd_e_delay();
    data = 0;
    if ( PIN(LCD_DATA0_PORT) & _BV(LCD_DATA0_PIN) ) data |= 0x10;
    if ( PIN(LCD_DATA1_PORT) & _BV(LCD_DATA1_PIN) ) data |= 0x20;
    if ( PIN(LCD_DATA2_PORT) & _BV(LCD_DATA2_PIN) ) data |= 0x40;
    if ( PIN(LCD_DATA3_PORT) & _BV(LCD_DATA3_PIN) ) data |= 0x80;
    lcd_e_low();

    lcd_e_delay();                           /* Enable 500ns low       */

    /* read low nibble */
    lcd_e_high();
    lcd_e_delay();
    if ( PIN(LCD_DATA0_PORT) & _BV(LCD_DATA0_PIN) ) data |= 0x01;
    if ( PIN(LCD_DATA1_PORT) & _BV(LCD_DATA1_PIN) ) data |= 0x02;
    if ( PIN(LCD_DATA2_PORT) & _BV(LCD_DATA2_PIN) ) data |= 0x04;
    if ( PIN(LCD_DATA3_PORT) & _BV(LCD_DATA3_PIN) ) data |= 0x08;
    lcd_e_low();

    return data;
}


/*************************************************************************
loops while lcd is busy, returns address counter
*************************************************************************/
static uint8_t lcd_waitbusy(void)

{
    register uint8_t c;

	if (lcd_autodetect)
	{
		/* wait until busy flag is cleared */
		uint16_t timeout = 4000;

		while ( ((c=lcd_read(0)) & (1<<LCD_BUSY)) && --timeout) {}

		/* the address counter is updated 4us after the busy flag is cleared */
		delay(2);

		/* now read the address counter */
		return (lcd_read(0));  // return address counter
	} else return 0;

}/* lcd_waitbusy */


/*************************************************************************
Move cursor to the start of next line or to the first line if the cursor
is already on the last line.
*************************************************************************/
static inline void lcd_newline(uint8_t pos)
{
    register uint8_t addressCounter;

	if (lcd_autodetect)
	{
    if ( pos < (LCD_START_LINE2) )
        addressCounter = LCD_START_LINE2;
    else
        addressCounter = LCD_START_LINE1;
    lcd_command((1<<LCD_DDRAM)+addressCounter);
	}
}/* lcd_newline */


/*
** PUBLIC FUNCTIONS
*/

/*************************************************************************
Send LCD controller instruction command
Input:   instruction to send to LCD controller, see HD44780 data sheet
Returns: none
*************************************************************************/
void lcd_command(uint8_t cmd)
{
	if (lcd_autodetect)
	{
	    lcd_waitbusy();
		lcd_write(cmd,0);
	}
}


/*************************************************************************
Send data byte to LCD controller
Input:   data to send to LCD controller, see HD44780 data sheet
Returns: none
*************************************************************************/
void lcd_data(uint8_t data)
{
	if (lcd_autodetect)
	{
	    lcd_waitbusy();
		lcd_write(data,1);
	}
}



/*************************************************************************
Set cursor to specified position
Input:    x  horizontal position  (0: left most position)
          y  vertical position    (0: first line)
Returns:  none
*************************************************************************/
void lcd_gotoxy(uint8_t x, uint8_t y)
{
	if (lcd_autodetect)
	{

    if ( y==0 )
        lcd_command((1<<LCD_DDRAM)+LCD_START_LINE1+x);
    else
        lcd_command((1<<LCD_DDRAM)+LCD_START_LINE2+x);
	}

}/* lcd_gotoxy */


/*************************************************************************
Clear display and set cursor to home position
*************************************************************************/
void lcd_clrscr(void)
{
	if (lcd_autodetect)
	{
		lcd_command(1<<LCD_CLR);
	}
}


/*************************************************************************
Display character at current cursor position
Input:    character to be displayed
Returns:  none
*************************************************************************/
void lcd_putc(char c)
{
    uint8_t pos;

	if (lcd_autodetect)
	{
		pos = lcd_waitbusy();   // read busy-flag and address counter
		if (c=='\n')
		{
			lcd_newline(pos);
		}
		else
		{
			lcd_write(c, 1);
		}
	}
}/* lcd_putc */


/*************************************************************************
Display string without auto linefeed
Input:    string to be displayed
Returns:  none
*************************************************************************/
void lcd_puts(const char *s)
/* print string on lcd (no auto linefeed) */
{
    register char c;

	if (lcd_autodetect)
	{
	    while ( (c = *s++) ) {
		    lcd_putc(c);
		}
	}

}/* lcd_puts */


/*************************************************************************
Display string from program memory without auto linefeed
Input:     string from program memory be be displayed
Returns:   none
*************************************************************************/
void lcd_puts_p(const char *progmem_s)
/* print string from program memory on lcd (no auto linefeed) */
{
    register char c;

	if (lcd_autodetect)
	{
	    while ( (c = pgm_read_byte(progmem_s++)) ) {
		    lcd_putc(c);
		}
	}

}/* lcd_puts_p */


/*************************************************************************
Initialize display and select type of cursor
Input:    dispAttr LCD_DISP_OFF            display off
                   LCD_DISP_ON             display on, cursor off
                   LCD_DISP_ON_CURSOR      display on, cursor on
                   LCD_DISP_CURSOR_BLINK   display on, cursor on flashing
Returns:  none
*************************************************************************/
void lcd_init(uint8_t dispAttr)
{
	register uint8_t c;

    /* Initialize LCD to 4 bit I/O mode, configure all lines as output */
    DDR(LCD_RS_PORT)    |= _BV(LCD_RS_PIN);
    DDR(LCD_RW_PORT)    |= _BV(LCD_RW_PIN);
    DDR(LCD_E_PORT)     |= _BV(LCD_E_PIN);
    DDR(LCD_DATA0_PORT) |= _BV(LCD_DATA0_PIN);
    DDR(LCD_DATA1_PORT) |= _BV(LCD_DATA1_PIN);
    DDR(LCD_DATA2_PORT) |= _BV(LCD_DATA2_PIN);
    DDR(LCD_DATA3_PORT) |= _BV(LCD_DATA3_PIN);

    delay(16000);        /* wait 16ms or more after power-on       */

    /* initial write to lcd is 8bit */
    LCD_DATA1_PORT |= _BV(LCD_DATA1_PIN);  // _BV(LCD_FUNCTION)>>4;
    LCD_DATA0_PORT |= _BV(LCD_DATA0_PIN);  // _BV(LCD_FUNCTION_8BIT)>>4;
    lcd_e_toggle();
    delay(4992);         /* delay, busy flag can't be checked here */

    /* repeat last command */
    lcd_e_toggle();
    delay(64);           /* delay, busy flag can't be checked here */

    /* repeat last command a third time */
    lcd_e_toggle();
    delay(64);           /* delay, busy flag can't be checked here */

    /* now configure for 4bit mode */
    LCD_DATA0_PORT &= ~_BV(LCD_DATA0_PIN);   // LCD_FUNCTION_4BIT_1LINE>>4
    lcd_e_toggle();
    delay(64);           /* some displays need this additional delay */

    /* from now the LCD only accepts 4 bit I/O, we can use lcd_command() */

	// andi6510: autodetect if LCD is connected
	lcd_autodetect = 0;
	lcd_write(LCD_FUNCTION_DEFAULT,0); /* function set: display lines  */
    c = lcd_read(0) & (1<<LCD_BUSY);
	if (c != 0) // busy flag high 
	{
		delay(64); // delay rather than waiting for busy flag
	    c = lcd_read(0) & (1<<LCD_BUSY);
		if (c == 0) // busy flag low
		{
			// busyflag went up then down again - there must be a LCD attached
			lcd_autodetect = 1;
		} 	
	}

	if (lcd_autodetect == 0) 
	{
		return; // seems no display is attached!
	}
	else
	{
		// andi6510: now autodetect controller type
	    lcd_command(LCD_MODE_DEFAULT); // set entry mode             
		lcd_command(0x28); // activate instruction table 0 - this command will only have an effect on the ST7036 controller
		lcd_command(0x40); // write CGRAM
		lcd_waitbusy();    // wait until display is ready
		lcd_data(0x07);    // write a 7 to the first byte of CGRAM
		lcd_command(0x29); // activate instruction table 1 - this command will only have an effect on the ST7036 controller
		lcd_command(0x40); // write CGRAM on HD44780 or ICONRAM on ST7036
		lcd_waitbusy();    // wait until display is ready
		lcd_data(0x04);    // write a 4 to the first byte of CGRAM or ICONRAM
		lcd_command(0x28); // activate instruction table 0 - this command will only have an effect on the ST7036 controller
		lcd_command(0x40); // read CGRAM on HD44780 and ST7036
		lcd_waitbusy();    // wait until display is ready
		lcd_autodetect = lcd_read(1); // read back CGRAM
	}

	switch(lcd_autodetect)
	{
	default:
	case 4:
		// is is not ST7036 so we assume that it is HD44780
		break;

	case 7: // we have auto detected the ST7036 controller!

		// andi6510: special initialisation for DOG module:
		lcd_command(0x29); // activate instruction table 1
		lcd_command(0x1c); // bias 1/4, FX=0
		lcd_command(0x52); // icon off, booster off, contrast 10....
		lcd_command(0x73); // contrast ...0011 -> 0x23 = 35 
		lcd_command(0x69); // follower on, amp ratio 1
		lcd_command(LCD_FUNCTION_DEFAULT); // activate instruction table 0
		break;
	
	case 0:
		// sorry, but seems no LCD is attached!
		break;
	}
	
	lcd_command(LCD_DISP_OFF);              /* display off                  */
    lcd_clrscr();                           /* display clear                */
    lcd_command(LCD_MODE_DEFAULT);          /* set entry mode               */
    lcd_command(dispAttr);                  /* display/cursor control       */

}/* lcd_init */

uint8_t lcd_controller_type(void)
{
	return lcd_autodetect;
}
