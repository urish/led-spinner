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

#define ARRAY_ENTRIES(array) (sizeof(array)/sizeof((array)[0]))
#define BV(value) (1 << (value))

#define BEEP_HIGH (0x40)
#define BEEP_LOW (0)

const int counter_cycle = 200;
const int counter_resolution = BV(CS10); /* prescale 1 */

int g_bitmap_pointer = 0;

SIGNAL(SIG_OVERFLOW1)
{
	TCNT1 = ~counter_cycle;

	g_bitmap_pointer++;
	g_bitmap_pointer %= ARRAY_ENTRIES(g_bitmap);
	PORTA = g_bitmap[g_bitmap_pointer];
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

	DDRA = 0xff;

	/* Setup timer 1 */
	TIFR	= BV(TOV1);
	TCCR1B	= counter_resolution;
	TCNT1	= ~counter_cycle;
	TIMSK	= BV(TOIE1);
	sei();

	/* Play the first row */
	PORTA = g_bitmap[g_bitmap_pointer];

	/* Enter sleep mode */
	beep(255, 50);
	for (;;);
}
