// ***********************************************************
// Project: Motor Control
// Author: Uri Shaked
// Module description: A simple program to control stepper motors.
// Revision: $Id$
// ***********************************************************

#include <avr/io.h>              // Most basic include files
#include <avr/interrupt.h>       // Add the necessary ones
#include <avr/signal.h>          // here
#include <util/twi.h>
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
	uint8 size;
	uint8 pointer;
	char data[32];
} io_buffer;

typedef struct
{
	uint8 cmd;
	union {
		uint8 beep_constant;
		uint16 counter_cycle;
		uint8 resolution_index;
		struct {
			uint8 size;
			uint8 data[1];
		} buffer;
	};
} motor_cmd;

typedef struct
{
	uint8 resolution;
	uint16 counter_cycle;
	struct {
		uint8 size;
		uint8 data[16];
	} buffer;
} motor_report;

/*** TWI-Related constants ***/
#define TWSR_CONTROL_MASK				(0xF8)

volatile uint8	buffer[16] 		= {0};
volatile uint8	buffer_len 		= 0;
volatile uint8	buffer_ptr		= 0;
volatile uint16	counter_cycle	= 25000;//8300 = 600RPM;//12500;
volatile uint8	accel_rate		= 0;

io_buffer i2c_recv_buffer = {0};
io_buffer i2c_send_buffer = {0};

void beep (unsigned char cycles, unsigned char pitch);

// Define here the global static variables
//

SIGNAL(SIG_OVERFLOW1)
{
	TCNT1 = ~counter_cycle;

	buffer_ptr++;
	buffer_ptr %= buffer_len;
	PORTA = buffer[buffer_ptr];
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

void i2c_init(void)
{
	/* Pullup I2C port */
	DDRC = 0x0;
	PORTC = 0xff;
	
	TWCR	= BV(TWEA) | BV(TWEN);
	TWAR	= 0x15 << 1;
}

void i2c_wait(void)
{
	while (!(TWCR & BV(TWINT)));
}

void i2c_handler(void) 
{

	if (!(TWCR & BV(TWINT)))
	{
		return;
	}
	
	/* Set the interrupt flag (according to AVR docs, clearing it is writing
		logic one to it) */
	TWCR &= ~BV(TWINT);
	
	switch (TWSR & TWSR_CONTROL_MASK)
	{
	/* Slave Receiver */
	case TW_SR_SLA_ACK:
	case TW_SR_ARB_LOST_SLA_ACK:
		/* Transmission start in Slave-Receive mode */
		i2c_recv_buffer.pointer = 0;
		i2c_recv_buffer.size	= 0;
		break;
	
	case TW_SR_DATA_ACK:	
		/* Acknowledged data byte received. */
		// XXX avoid overflow
		i2c_recv_buffer.data[i2c_recv_buffer.pointer] = TWDR;
		i2c_recv_buffer.pointer++;
		break;
	
	case TW_SR_STOP:
		/* STOP or Repeated-Start condition */
		i2c_recv_buffer.size	= i2c_recv_buffer.pointer;
		TWCR |= BV(TWEA);
		break;
	
	/* Slave Transmitter */	
	case TW_ST_SLA_ACK:
	case TW_ST_ARB_LOST_SLA_ACK:
		TWDR = i2c_send_buffer.size;
		i2c_send_buffer.pointer = 0;
		TWCR |= BV(TWEA);
		break;
	
	case TW_ST_DATA_ACK:
		TWDR = i2c_send_buffer.data[i2c_send_buffer.pointer];
		i2c_send_buffer.pointer++;
		break;
			
	case TW_ST_DATA_NACK:
		break;
		
	case TW_ST_LAST_DATA:
		i2c_send_buffer.pointer = 0;
		TWCR |= BV(TWEA);
		break;
	}
	
	// Release the TWI Bus.
	TWCR |= BV(TWINT);
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
	motor_cmd * command = NULL;
	motor_report * report = (motor_report*)i2c_send_buffer.data;
	
	beep(255, 50);
	
	memcpy((void*)buffer, sequence, sizeof(sequence));
	buffer_len = sizeof(sequence) / sizeof(sequence[0]);
	
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
	
	i2c_init();

	for (;;)
	{
		i2c_handler();
		
		if (!i2c_recv_buffer.size) {
			continue;
		}
		
		command = (motor_cmd*)i2c_recv_buffer.data;
		
		switch (command->cmd)
		{
		case 0:
			beep(120, command->beep_constant);
			break;
		
		case 1:
			counter_cycle = command->counter_cycle;
			break;
		
		case 2:
			resolution = command->resolution_index % ARRAY_ENTRIES(resolution_table);
			TCCR1B = resolution_table[resolution];
			break;
		
		case 3:
			buffer_len = command->buffer.size;
			for (j = 0; j < buffer_len; j++)
			{
				buffer[j] = command->buffer.data[j];
			}
			break;

		case 4:
			i2c_send_buffer.size = sizeof(*report);
			report->counter_cycle = counter_cycle;
			report->resolution	 = resolution;
			report->buffer.size	 = buffer_len;			
			for (j = 0; j < buffer_len; j++)
			{
				report->buffer.data[j] = buffer[j];
			}
			break;
		}
	}
}
