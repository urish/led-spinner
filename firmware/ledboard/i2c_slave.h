/*
 * Revision: $Id$
 */

#ifndef __I2C_SLAVE_H
#define __I2C_SLAVE_H

/* Constants */
#define I2C_MAX_BUFFER_SIZE		(24)

/* Typedefs */
typedef struct {
	/* 'buf' must be first for more code size-optimized bootloader */
	unsigned char buf[I2C_MAX_BUFFER_SIZE];	
	unsigned char size;
	unsigned char pointer;
} i2c_buffer;

/* Globals */
extern i2c_buffer i2c_incoming;
extern i2c_buffer i2c_outgoing;

/* Prototypes */
extern void i2c_slave_setup(unsigned char device_address);
extern void i2c_slave_handler();

#endif /* __I2C_SLAVE_H */
