// ***********************************************************
// Project: LedScreen
// Module description: Main Module
// Author: Uri Shaked
// Revision: $Id$
// ***********************************************************

/* Includes */
#include <pic18fregs.h>
#include <string.h>
#include "i2c_slave.h"
#include "text.h"

/* Constants */
#pragma stack 0x200 100

// The address of the device on the I2C bus
#define I2C_DEVICE_ADDRESS	(0x11)

// Timer counter cycle
#define TIMER_COUNTER_CYCLE		(49911)
#define TIMER_PRESCALER_VALUE	(0x5)

const char source_revision[] = "$Rev$";

enum
{
	ANSWER_OK,
	ANSWER_ERROR_CHECKSUM,
	ANSWER_ERROR_INVALID_COMMAND,
};

/* Macros */
#define ARRAY_ENTRIES(arr) (sizeof(arr)/sizeof((arr)[0]))

/* Typedefs */
typedef unsigned char byte;
typedef unsigned int  word;
typedef unsigned long dword;

typedef struct
{
	byte checksum;
	byte command_code;
	byte buf[22];
} command_packet;

typedef struct
{
	byte checksum;
	byte error_code;
	byte buf[22];
} answer_packet;

typedef struct
{
	dword prev_counters[4];
	dword current_counter;
} counter_stat_buffer;

/* Global variables */
dword	g_int2_times[4];
byte	g_int2_times_index	 = 0;
byte	g_int2_counter_upper = 0;

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

/* Utility function: calculate_checksum() */
static byte calculate_checksum(data byte * buffer, byte size)
{
	byte sum = 0;
	for (; size; buffer++, size--)
	{
		sum += *buffer;
	}
	return ~sum + 1;
}

dword read_int2_counter(void)
{
	byte int2_counter_low = TMR1L;
	return ((dword)g_int2_counter_upper << 16) | 
			((dword)TMR1H << 8) | 
			int2_counter_low;
}

void handle_command(void)
{
	data command_packet * cmd = (data command_packet *)i2c_incoming.buf;
	data answer_packet  * answer = (data answer_packet *)i2c_outgoing.buf;
	data counter_stat_buffer * counter_stat	= NULL;
	byte i = 0;
	
	memset(answer, 0, sizeof(i2c_outgoing.buf));
	
	/* Validate incoming packet */
	if (calculate_checksum(i2c_incoming.buf, i2c_incoming.size))
	{
		answer->error_code = ANSWER_ERROR_CHECKSUM;
		goto finalize_answer;
	}
	
	answer->error_code = ANSWER_OK;
	switch (cmd->command_code)
	{
	case 0: /* Read version */
		strcpy(answer->buf, source_revision);
		break;
	
	case 1: /* Read data array */
		break;
	
	case 2: /* Write data array */
		break;
	
	case 3: /* Read speed stats */
		counter_stat = (data counter_stat_buffer *)answer->buf;
		for (i = 0; i < ARRAY_ENTRIES(counter_stat->prev_counters); i++)
		{
			counter_stat->prev_counters[i] =
				g_int2_times[(g_int2_times_index + i) % 
							 ARRAY_ENTRIES(g_int2_times)];
		}
		counter_stat->current_counter = read_int2_counter();
		break;
	
	default:
		answer->error_code = ANSWER_ERROR_INVALID_COMMAND;
	}
	
finalize_answer:
	i2c_outgoing.size = sizeof(i2c_outgoing.buf);
	answer->checksum = calculate_checksum((data byte *)answer, i2c_outgoing.size);
}

// ***********************************************************
// Main program
//
void main(void)
{
	/* Internal oscilator setup @ 8MHz */
	OSCCON = 0x7F;
	
	/* PortB digital IO */
	ADCON1 |= 0x0F;
	
	/* Pull up port B */
	INTCON2bits.RBPU = 0;
	
	/* Initialization */
	memset(g_int2_times, 0, sizeof(g_int2_times));
	
	/* Configure TMR0 */
	TMR0H	= TIMER_COUNTER_CYCLE >> 8;
	TMR0L	= TIMER_COUNTER_CYCLE & 0xff;
	T0CON	= TIMER_PRESCALER_VALUE & 0x7;
	T0CONbits.TMR0ON	= 1;
	INTCONbits.TMR0IE	= 1;
	INTCONbits.TMR0IF	= 0;
	
	/* Configure TMR1 */
	TMR1H	= 0;
	TMR1L	= 0;
	T1CONbits.RD16		= 1;	/* 16 bit read/write mode */
	T1CONbits.T1CKPS0	= 1; 	/* 8 bit prescaler */
	T1CONbits.T1CKPS1	= 1; 	/* 8 bit prescaler */
	T1CONbits.TMR1ON	= 1;
	PIE1bits.TMR1IE		= 0;
	PIR1bits.TMR1IF		= 0;
	
	/* Configure INT2 (on RB2) */
	TRISBbits.TRISB2	= 1; /* Configure INT2 pin as an input */
	INTCON2bits.INTEDG2	= 1;
	INTCON3bits.INT2IE 	= 0;
	INTCON3bits.INT2IF	= 0;
	
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
		if (i2c_incoming.size)
		{
			handle_command();
			i2c_incoming.size = 0;
		}
		if (PIR1bits.TMR1IF)
		{
			if (g_int2_counter_upper < 0xff)
			{
				g_int2_counter_upper++;
			}
			PIR1bits.TMR1IF = 0;
		}
		if (INTCON3bits.INT2IF)
		{
			/* Handle an INT2 event */
			g_int2_times[g_int2_times_index] = read_int2_counter();
			g_int2_times_index++;
			g_int2_times_index %= ARRAY_ENTRIES(g_int2_times);
			/* Reset the stats, interrupt flag */
			g_int2_counter_upper = 0;
			TMR1H = 0;
			TMR1L = 0;
			INTCON3bits.INT2IF = 0;
		}
	}
}
