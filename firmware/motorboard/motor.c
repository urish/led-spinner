// ***********************************************************
// Project: Motor Control
// Author: Uri Shaked
// Module description: A simple program to control stepper motors.
// Revision: $Id$
// ***********************************************************

#include <avr/io.h>              // Most basic include files
#include <avr/interrupt.h>       // Add the necessary ones
#include <avr/signal.h>          // here
#include <string.h>

#define BEEP_HIGH (0x40)
#define BEEP_LOW (0)

#define DD_AUX0 (PB3)
#define DD_AUX1 (PB4)

#define DD_MISO (PB6)
#define DDR_SPI (DDRB)
#define DD_SCK (PB7)
#define DD_MOSI (PB5)

#define PINS_CONNECTED_MAGIC (0xa8)

#define BV(value) (1 << (value))

#define ARRAY_ENTRIES(x) (sizeof(x)/sizeof((x)[0]))

typedef unsigned char uint8;
typedef unsigned long uint16;
typedef unsigned char bool;

typedef struct
{
	char data[32];
	uint8 size;
	uint8 pointer;
} io_buffer;

/*** TWI-Related constants ***/
#define TWSR_CONTROL_MASK				(0xF8)

/* TWI status masks */
#define TWI_SLAVE_RECEIVE_START			(0x60)
#define TWI_SLAVE_RECEIVE_DATA			(0x80)
#define TWI_SLAVE_RECEIVE_STOP			(0xA0)
#define TWI_SLAVE_TRANSMIT_START		(0xA8)
#define TWI_SLAVE_TRANSMIT_ACK_SEND		(0xB8)
#define TWI_SLAVE_TRANSMIT_NACK_SEND	(0xC0)
#define TWI_SLAVE_TRANSMIT_FINISHED		(0xC8)

volatile uint8	buffer[20] 		= {0};
volatile uint8	buffer_len 		= 0;
volatile uint8	buffer_ptr		= 0;
volatile uint16	counter_cycle	= 8300;//12500;
volatile uint8	accel_rate		= 0;
volatile bool	sense_mode 		= 0;

void beep (unsigned char cycles, unsigned char pitch);

// Define here the global static variables
//

SIGNAL(SIG_OVERFLOW1)
{
	TCNT1 = ~counter_cycle;

	buffer_ptr++;
	buffer_ptr %= buffer_len;
	PORTA = buffer[buffer_ptr];

	if (sense_mode)
	{
		PORTB = ~PORTB;
	}
}

void beep (unsigned char cycles, unsigned char pitch) 
{
	char i;
	
	DDRD |= 1 << PD6;			// Make PD6 an output

	while (cycles > 0) 
	{
		PORTD |= 1 << PD6;		// buzzer pin high
		for(i = 0; i < pitch; i++);
		PORTD &= ~(1 << PD6);		// buzzer pin low
		for(i = 0; i < pitch; i++);
		cycles--;
	}
	
	DDRD &= ~(1 << PD6);		// Disable PD6
	
	return;
}

int pins_connected(uint8 pin1, uint8 pin2)
{
	uint8 magic = PINS_CONNECTED_MAGIC;
	uint8 bit	= 0;
	uint8 index = 0;
	uint8 old_ddrb = DDRB;
	
	DDRB |= BV(pin1);
	DDRB &= ~BV(pin2);
	PORTB |= BV(pin2);
	
	for (index = 0; index < 8; index++) {
		bit = magic & 0x1;
		if (bit)
		{
			PORTB |= BV(pin1);
		} 
		else
		{
			PORTB &= ~BV(pin1);
		}
		asm("NOP");
		if ((PINB & BV(pin2)) != (bit * BV(pin2)))
		{
			DDRB = old_ddrb;
			return 0;
		}
		magic >>= 1;
	}
	
	DDRB = old_ddrb;
	
	return 1;
}

unsigned char spi_read_byte(void) 
{
	while (!(SPSR & (1 << SPIF)));
	
	return SPDR;
}

unsigned long spi_read_word(void)
{
	return (spi_read_byte() << 8) | spi_read_byte();
}

void spi_write_byte(unsigned char value)
{
	SPDR = value;
	while (!(SPSR & (1 << SPIF)));
}

void spi_write_word(unsigned long value)
{
	spi_write_byte(value >> 8);
	spi_write_byte(value & 0xff);
}

uint8 jumper_mode_read_jumpers()
{
	if (pins_connected(DD_AUX0, DD_AUX1)) {
		if (pins_connected(DD_MOSI, DD_MISO)) {
			return 5;
		}
		return 1;
	}
	if (pins_connected(DD_AUX1, DD_MOSI)) {
		return 2;
	}
	if (pins_connected(DD_MOSI, DD_MISO)) {
		return 3;
	}
	if (pins_connected(DD_MISO, DD_SCK)) {
		return 4;
	}
	return 0;
}

void wait100ms()
{
	uint16 i = 0;
	uint8 j = 0;
	/* about 0.1 seconds */
	for (i = 0; i < 40000; i++)
	{
		for (j = 0; j < 20; j++);
	}
}

void accel_fast()
{
	uint8 i = 0;
	uint16 j = 0;

	counter_cycle = 30000;
	/* wait 0.4 sec */
	for (i = 0; i < 4; i++) 
	{
		wait100ms();
	}

	counter_cycle = 20000;
	wait100ms();

	counter_cycle = 15000;
	wait100ms();

	counter_cycle = 12000;
	wait100ms();

	counter_cycle = 10000;
	wait100ms();

	for (j = 10000; j >= 6000; j -= 100)
	{
		counter_cycle = j;
		wait100ms();
	}
}

void accel_slow()
{
	uint8 i = 0;
	uint16 j = 0;

	counter_cycle = 40000;
	/* wait 1 sec */
	for (i = 0; i < 10; i++) 
	{
		wait100ms();
	}
	
	for (j = 39000; j > 14000; j -= 1000)
	{
		counter_cycle = j;
		wait100ms();
	}

	for (j = 15000; j > 7000; j -= 100)
	{
		counter_cycle = j;
		wait100ms();
	}

	for (j = 7000; j >= 6000; j -= 50)
	{
		counter_cycle = j;
		wait100ms();
	}
}

void jumper_mode()
{
	uint8 mode = jumper_mode_read_jumpers();
	uint8 new_mode = 0;
	
	beep(255, 20 + mode * 5);
	
	for (;;)
	{
		new_mode = jumper_mode_read_jumpers();
		if (new_mode && (mode != new_mode)) {
			mode = new_mode;
			beep(255, 20 + mode * 5);
			switch (mode)
			{
			case 1:
				counter_cycle = 25000;
				break;
			
			case 2:
				counter_cycle = 50000;
				break;
			
			case 3:
				accel_fast();
				break;
			
			case 4:
				accel_slow();
				break;

			case 5:
				counter_cycle = 12000;
				break;
			}
		}
	}
}

// ***********************************************************
// Main program
//
int main(void) 
{
	uint8	j = 0;
	const uint8 sequence[] = {2, 4, 1};
	const uint8 resolution_table[] = 
	{
		BV(CS10),				/* prescale 1 */
		BV(CS11),				/* prescale 8 */
		BV(CS11) | BV(CS10),	/* prescale 64 */
		BV(CS12),				/* prescale 256 */
		BV(CS12) | BV(CS10)		/* prescale 1024 */
	};
	uint8	resolution	= 0;
	uint8	cmd			= 0;
	
	beep(255, 50);
	
	memcpy((void*)buffer, sequence, sizeof(sequence));
	buffer_len = sizeof(sequence)/sizeof(sequence[0]);
	
	DDRA = 0xf;
	PORTB= 0x0;
	DDRB = 1 | (1 << DD_MISO);

	
	/* Setup timer 1 */
	TIFR	= BV(TOV1);
	TCCR1B	= resolution_table[resolution];
	TCNT1	= ~counter_cycle;
	TIMSK	= BV(TOIE1);
	sei();
	
	/* Check for jumper-control mode */
	if (pins_connected(DD_AUX0, DD_AUX1)) 
	{
		jumper_mode();
	}
	
	DDRB = 0;
	PORTB = 0;
	
	/* Pullup I2C port */
	DDRC = 0x0;
	PORTC = 0xff;
	
	TWCR	= BV(TWEA) | BV(TWEN);
	TWAR	= 0x15 << 1;

	while (cmd != 0xfa)
	{
		while (!(TWCR & BV(TWINT)));
	
		switch (TWSR & TWSR_CONTROL_MASK)
		{
		case TWI_SLAVE_RECEIVE_START:		
			/* Transmission start in Slave-Receive mode */
			j = 0;
			break;
		
		case TWI_SLAVE_RECEIVE_DATA:
			/* Acknowledged data byte received. */
			buffer[j++] = TWDR;
			break;
		
		case TWI_SLAVE_RECEIVE_STOP:  /* STOP or Repeated-Start condition */
			TWCR |= BV(TWEA);
			break;
			
		case TWI_SLAVE_TRANSMIT_START:
			TWDR = 0xcc;
			TWCR |= BV(TWEA);
			break;
		
		case TWI_SLAVE_TRANSMIT_ACK_SEND:
			break;
			
		case TWI_SLAVE_TRANSMIT_NACK_SEND:
			break;
			
		case TWI_SLAVE_TRANSMIT_FINISHED:
			TWCR |= BV(TWEA);
			break;
		}
		
		// Release the TWI Bus.
		TWCR |= BV(TWINT);
	}

	TWCR	= 0;

	/* Set MISO output, all others input */
	DDR_SPI = (1 << DD_MISO);

	/* Enable SPI */
	SPCR = (1 << SPE);
	for (;;)
	{
		while (!(SPSR & (1 << SPIF)));
		
		/* Get Control */
		cmd = SPDR;
		
		spi_write_byte(0xae);
		spi_write_byte(0x0);
				
		switch (cmd)
		{
		case 0:
			beep(120, spi_read_byte());
			break;
					
		case 1:
			counter_cycle = spi_read_word();
			break;
				
		case 2:
			resolution = spi_read_byte() % ARRAY_ENTRIES(resolution_table);
			TCCR1B = resolution_table[resolution];
			break;
		
		case 3:
			buffer_len = spi_read_byte();
			for (j = 0; j < buffer_len; j++)
			{
				buffer[j] = spi_read_byte();
			}
			break;
					
		case 4:
			spi_write_word(counter_cycle);
			spi_write_byte(resolution);
			spi_write_byte(buffer_len);
			for (j = 0; j < buffer_len; j++)
			{
				spi_write_byte(buffer[j]);
			}
			break;

		case 7:
			/* Enter sense mode */
			SPCR &= ~(1 << SPE);
			sense_mode = 1;
			for (;;);
		}

		while (SPSR & (1 << SPIF))
		{
			SPDR = 0;
		}
	}
}
