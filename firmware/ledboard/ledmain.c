// ***********************************************************
// Project: LedScreen
// Module description: Main Module
// Author: Uri Shaked
// Revision: $Id$
// ***********************************************************

#include <avr/io.h>		 	// Most basic include files
#include <avr/interrupt.h>	 // Add the necessary ones
#include <avr/signal.h> 	 // here
#include "text.h"

// The address of the device on the I2C bus
#define I2C_DEVICE_ADDRESS	(0x11)		

#define ARRAY_ENTRIES(array) (sizeof(array)/sizeof((array)[0]))
#define BV(value) (1 << (value))

#define BEEP_HIGH (0x40)
#define BEEP_LOW (0)

const int counter_cycle = 1667;
const int counter_resolution = BV(CS10); /* prescale 1 */

int g_bitmap_pointer = 0;

SIGNAL(SIG_OVERFLOW1)
{
	TCNT1 = ~counter_cycle;

	g_bitmap_pointer++;
	g_bitmap_pointer %= ARRAY_ENTRIES(g_bitmap);
	PORTA = g_bitmap[g_bitmap_pointer];
}

SIGNAL(SIG_INTERRUPT1)
{
	g_bitmap_pointer = 0;
	TCNT1 = ~counter_cycle;
}

void beep (unsigned char cycles, unsigned char pitch) {
	char i;

	DDRD |= 1 << PD6;			// Make PD6 an output

	while (cycles > 0) {
		PORTD |= 1 << PD6;		// buzzer pin high
		for(i = 0; i < pitch; i++);
		PORTD &= ~(1 << PD6);		// buzzer pin low
		for(i = 0; i < pitch; i++);
		cycles--;
	}

	DDRD &= ~(1 << PD6);		// Disable PD6

	return;
}

// ***********************************************************
// Main program
//
int main(void) {
	int i = 0;

	DDRA = 0xff;

	/* Setup timer 1 */
	TIFR	= BV(TOV1);
	TCCR1B	= counter_resolution;
	TCNT1	= ~counter_cycle;
	TIMSK	= BV(TOIE1);
	
	/* Setup INT 1 */
	DDRD	&= ~BV(PD3);
	PORTD	|= BV(PD3);
	MCUCR	= (MCUCR & 0xf0) | BV(ISC11);
	GICR	|= BV(INT1);
	
	/* Initialize the I2C / TWI bus in Slave Mode */
	TWCR	= BV(TWEA) | BV(TWEN);
	TWAR	= (I2C_DEVICE_ADDRESS << 1);
	
	/* Enable interrupts */
	sei();

	/* Play the first row */
	PORTA = g_bitmap[g_bitmap_pointer];

	/* Startup beep (to be removed) */
	beep(255, 50);
	
	/* I2C Bus Polling */
	for (;;) {
		if (!(TWCR & BV(TWINT))) {
			continue;
		}
		
		// We were addressed by the I2C bus; Handle this.
		switch (TWSR & 0xF8) {
		case 0x60:	/* Transmission start in Slave-Receive mode */
			/* Do Nothing */
			break;
		
		case 0x80:	/* Acknowledged data byte received. */
			/* Copy the received data all over the bitmap */
			for (i = 0; i < ARRAY_ENTRIES(g_bitmap); i++) {
				g_bitmap[i] = TWDR;
			}
			break;
			
		case 0xA8:	/* Transmission start in Slave-Transmit mode */
			break;
		
		case 0xB8:	/* Acknowledged data byte sent */
			break;
			
		case 0xC0:	/* Non-acknowledged data byte sent */
			break;
			
		case 0xC8:	/* Last data byte was sent */
			break;
		}
		
		// Release the TWI Bus.
		TWCR &= ~BV(TWINT);
	}
}
