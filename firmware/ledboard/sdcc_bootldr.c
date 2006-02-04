/*
 * Revision: $Id$
 * Tab Width: 4
 * Purpose: A hack that relocates RESET interrupt vector to 0x800.
 *			Useful with Microchip's USB Bootloader.
 */

#pragma code crt_dispatch 0x800
void crt_dispatch () __naked
{
	__asm extern __startup __endasm;
	__asm goto __startup __endasm;
}
