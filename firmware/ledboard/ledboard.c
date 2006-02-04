// ***********************************************************
// Project: LedScreen
// Module description: Main Module
// Author: Uri Shaked
// Revision: $Id$
// ***********************************************************

/* Includes */
#include <pic18fregs.h>
#include "i2c_slave.h"
#include "text.h"

/* Constants */
#pragma stack 0x200 100

// The address of the device on the I2C bus
#define I2C_DEVICE_ADDRESS	(0x11)

// Timer counter cycle
#define TIMER_COUNTER_CYCLE		(49911)
#define TIMER_PRESCALER_VALUE	(0x0)

/* Macros */
#define ARRAY_ENTRIES(array) (sizeof(array)/sizeof((array)[0]))
#define BV(value) (1 << (value))

const int counter_cycle = 1100;

int g_bitmap_pointer = 0;
int is_hidden = 1;

/* Interrupt handler */
void interrupt1_handler() interrupt 1
{
	if (INTCONbits.TMR0IF)
	{
		PORTBbits.RB3 ^= 1;
		TMR0H	= TIMER_COUNTER_CYCLE >> 8;
		TMR0L	= TIMER_COUNTER_CYCLE & 0xff;
		INTCONbits.TMR0IF	= 0;
	}
}

// ***********************************************************
// Main program
//
void main(void) {
	/* Internal oscilator setup @ 8MHz */
	OSCCON = 0x7F;
	
	/* PortB digital IO */
	ADCON1 |= 0x0F;
	
	/* Configure TMR0 */
	TMR0H	= TIMER_COUNTER_CYCLE >> 8;
	TMR0L	= TIMER_COUNTER_CYCLE & 0xff;
	T0CON	= TIMER_PRESCALER_VALUE & 0x7;
	T0CONbits.TMR0ON	= 1;
	INTCONbits.TMR0IE	= 1;
	INTCONbits.TMR0IF	= 0;
	
	/* Enable interrupts */
	INTCONbits.GIE = 1;
	
	/* Configure RB3 as an output */
	TRISBbits.TRISB3 = 0;

	/* Setup I2C slave */
	i2c_slave_setup(I2C_DEVICE_ADDRESS);

	/* Main loop */
	for (;;)
	{
		i2c_slave_handler();
	}
}
