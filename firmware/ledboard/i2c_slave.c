/*
 * Revision: $Id$
 */

/* Includes */
#include <pic18fregs.h>
#include "i2c_slave.h"

/* Constants */
/* I2C Slave modes */
#define I2C_MODE_SLAVE10B_INT	(0x0f)
#define I2C_MODE_SLAVE7B_INT	(0x0e)
#define I2C_MODE_SLAVE10B		(0x07)
#define I2C_MODE_SLAVE7B		(0x06)
#define I2C_SLEW_OFF			(0xc0)

/* I2C Slave state mask */
#define I2C_STATE_MASK			(0x3d)
#define I2C_STATE_RECV_START	(0x09)	/* AN734/State 1 */
#define I2C_STATE_RECV_DATA		(0x29)	/* AN734/State 2 */
#define I2C_STATE_RECV_EOF		(0x30)	/* Not defined in AN734 */
#define I2C_STATE_XMIT_START	(0x0d)	/* State is not as documented (0x0c) ! */
#define I2C_STATE_XMIT_DATA		(0x2c)	/* AN734/State 4 */
#define I2C_STATE_XMIT_EOF		(0x28)	/* AN734/State 5 */

/* Globals */
i2c_buffer i2c_incoming;
i2c_buffer i2c_outgoing;

/* Setups the PIC device to act as I2C slave with the given address */
void i2c_slave_setup(unsigned char device_address)
{
	/* Reset I2C state */
	SSPSTAT &= 0x3f;
	SSPCON1  = 0;
	SSPCON2  = 0;

	/* Select I2C slave mode */
	SSPCON1 |= I2C_MODE_SLAVE7B_INT;

	/* Stretch-enable, No slew-rate control */
	SSPCON2bits.SEN = 1;
	SSPSTAT |= I2C_SLEW_OFF;

	/* Pin tristate configuration (as Inputs) */
	TRISBbits.TRISB1 = 1;
	TRISBbits.TRISB0 = 1;

	SSPADD = device_address << 1;
	
	/* Enable MSSP module */
	SSPCON1 |= 0x20;
	
	/* Reset I2C buffers */
	i2c_incoming.size = 0;
	i2c_incoming.pointer = 0;
	i2c_outgoing.size = 0;
	i2c_outgoing.pointer = 0;
}

void i2c_slave_handler() 
{
	unsigned char in_byte  = 0;
	unsigned char out_byte = 0;

	if (!PIR1bits.SSPIF)
	{
		return;
	}
		
	/* Something happend, deal with it */
	switch (SSPSTAT & I2C_STATE_MASK)
	{
	case I2C_STATE_RECV_START:
		i2c_incoming.pointer = 0;
		i2c_incoming.size = 0;
		in_byte = SSPBUF;
		if (SSPCON1bits.SSPOV)
		{
			SSPCON1bits.SSPOV = 0;
			in_byte = SSPBUF;
		}
		break;
	
	case I2C_STATE_RECV_DATA:
		in_byte = SSPBUF;
		if (i2c_incoming.pointer < I2C_MAX_BUFFER_SIZE)
		{
			i2c_incoming.buf[i2c_incoming.pointer] = in_byte;
			i2c_incoming.pointer++;
		}
		if (SSPCON1bits.SSPOV) {
			SSPCON1bits.SSPOV = 0;
			in_byte = SSPBUF;
		}
		break;
		
	case I2C_STATE_RECV_EOF:
		i2c_incoming.size		= i2c_incoming.pointer;
		i2c_incoming.pointer	= 0;
		break;

	case I2C_STATE_XMIT_START:
		/* HACK (but I've found no other way to do this) - 
			Deal with a repeated start condition, after slave RECV */
		if (i2c_incoming.pointer > 0)
		{
			i2c_incoming.size		= i2c_incoming.pointer;
			i2c_incoming.pointer	= 0;
			return;
		}
		
		in_byte = SSPBUF;
		i2c_outgoing.pointer = 0;
	case I2C_STATE_XMIT_DATA:
		while (SSPSTATbits.BF);		/* Loop while buffer is full */
		
		/* Determine the byte to send */
		out_byte = 0;
		if (i2c_outgoing.pointer < i2c_outgoing.size)
		{
			out_byte = i2c_outgoing.buf[i2c_outgoing.pointer];
			i2c_outgoing.pointer++;
		}
		
		/* Send the data */
		do
		{
			SSPCON1bits.WCOL = 0;		/* Clear write-collision flag */
			SSPBUF = out_byte;
		} while (SSPCON1bits.WCOL);
		
		break;

	case I2C_STATE_XMIT_EOF:
		i2c_outgoing.pointer = 0;
		break;
	}

	if (SSPCON1bits.SSPOV) {
		SSPCON1bits.SSPOV = 0;
		in_byte = SSPBUF;
	}

	/* Clean interrupt flag, disable clock streching */
	PIR1bits.SSPIF = 0;
	SSPCON1bits.CKP = 1;
}
