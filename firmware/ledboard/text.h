// Makefile for AVR project Led Screen Display
// Revision: $Id: Makefile 82 2005-10-23 11:07:25Z  $

#ifndef __TEXT_H_
#define __TEXT_H_

#define BITMAP_HEIGHT	= (5)
#define BITMAP_WIDTH	= (35)

static unsigned char g_bitmap[] = {
	0x1f, 0x04, 0x04, 0x04, 0x1f, 0x00, 0x00, 0x1f, 0x15, 0x15, 
	0x15, 0x15, 0x00, 0x00, 0x1f, 0x10, 0x10, 0x10, 0x10, 0x00, 
	0x00, 0x1f, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, 0x0e, 0x11, 
	0x11, 0x11, 0x0e, 0x00, 0x00, 
};

#endif /* __TEXT_H_ */
