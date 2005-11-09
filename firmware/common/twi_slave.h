// ***********************************************************
// Project: LSD
// Author: Uri Shaked
// Module description: A simple I2C (TWI) Slave Implementation
// Revision: $Id: motor.c 82 2005-10-23 11:07:25Z  $
// ***********************************************************

#ifndef __TWI_SLAVE_H__
#define __TWI_SLAVE_H__

/*** Includes ***/
#include "lsd_common.h"

/*** Constants ***/
#define TWI_IO_BUFFER_SIZE	(32)

/*** Typedefs ***/
typedef struct
{
	uint8 size;
	uint8 pointer;
	uint8 data[TWI_IO_BUFFER_SIZE];
} twi_io_buffer;

/*** Globals ***/
extern twi_io_buffer twi_input_buffer;
extern twi_io_buffer twi_output_buffer;

/*** Exported Functions ***/
/* XXX need to be documented */
void twi_init(uint8 my_address, bool broadcast_enable);
bool twi_is_ready(void);
void twi_poll();

#endif /* __TWI_SLAVE_H__ */
