// ***********************************************************
// Project: LSD
// Author: Uri Shaked
// Module description: Defines several common types & macroes
// Revision: $Id: motor.c 82 2005-10-23 11:07:25Z  $
// ***********************************************************

#ifndef __LSD_COMMON_H__
#define __LSD_COMMON_H__

/*** Constants ***/
#define false				(0)
#define true				(1)

/*** Macroes ***/
#define BV(bit_index)		(1 << (bit_index))
#define ARRAY_ENTRIES(x)	(sizeof(x)/sizeof((x)[0]))

/*** Typedefs ***/
typedef unsigned char uint8;
typedef unsigned long uint16;
typedef unsigned char bool;

#endif /* __LSD_COMMON_H__ */
