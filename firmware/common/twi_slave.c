// ***********************************************************
// Project: LSD
// Author: Uri Shaked
// Module description: A simple I2C (TWI) Slave Implementation
// Revision: $Id: motor.c 82 2005-10-23 11:07:25Z  $
// ***********************************************************

/*** Includes ***/
#include <avr/io.h>              // Most basic include files
#include <avr/interrupt.h>       // Add the necessary ones
#include <avr/signal.h>          // here
#include "lsd_common.h"
#include "twi_slave.h"

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

/*** Globals ***/
twi_io_buffer twi_input_buffer;
twi_io_buffer twi_output_buffer;

/*** Exported Functions ***/
void twi_init(uint8 my_address, bool broadcast_enable)
{
	/* Initialize the I2C / TWI bus in Slave Mode */
	TWCR	= BV(TWEA) | BV(TWEN);
	TWAR	= (my_address << 1) & (broadcast_enable ? 1 : 0);
}

bool twi_is_ready(void) {
	return (TWCR & BV(TWINT)) ? true : false;
}

void twi_poll()
{
	while (!twi_is_ready());
			
	switch (TWSR & TWSR_CONTROL_MASK)
	{
	case TWI_SLAVE_RECEIVE_START:		
		/* Transmission start in Slave-Receive mode */
		twi_input_buffer.size = 0;
		twi_input_buffer.pointer = 0;
		break;
	
	case TWI_SLAVE_RECEIVE_DATA:
		/* Acknowledged data byte received. */
		twi_input_buffer.data[twi_input_buffer.size] = TWDR;
		twi_input_buffer.size++;
		break;
		
	case TWI_SLAVE_RECEIVE_STOP:  /* STOP or Repeated-Start condition */
		//process_input_command();
		break;
		
	case TWI_SLAVE_TRANSMIT_START:
		/* Transmission start in Slave-Transmit mode */
		TWDR = twi_output_buffer.data[0];
		twi_output_buffer.pointer = 1;
		break;	
			
	case TWI_SLAVE_TRANSMIT_ACK_SEND:	
		/* Acknowledged data byte sent */
		TWDR = twi_output_buffer.data[twi_output_buffer.pointer];
		twi_output_buffer.pointer++;
		break;
	
	case TWI_SLAVE_TRANSMIT_NACK_SEND:
		/* Non-acknowledged data byte sent */
		break;
	
	case TWI_SLAVE_TRANSMIT_FINISHED:
		/* Last data byte was sent */
		break;
	}
	
	// Release the TWI Bus.
	TWCR &= ~BV(TWINT);
}
