/*
Copyright (C) 2023		nitr8

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/


#include "string.h"
#include "gecko.h"


#define BUILDSTR		__DATE__" "__TIME__


int _main(void)
{
	int i;

	//
	// Initialize the USB Gecko
	//
	gecko_init();

	gecko_printf("\n");
	gecko_printf("\n");
	gecko_printf("USB Gecko initialized\n");
	gecko_printf("\n");
	gecko_printf("============= FREE =============\n");
	gecko_printf("\n");

	gecko_printf("Freeing preloader area in memory...\n");

	for (i = 0; i < 0x10000; i++)
	{
		if (*(unsigned char *)(0x11400000 + i) != 0x00)
		{
#if DEBUG
			gecko_printf("Memory @ %08x reads %02x ", 0x11400000 + i, *(unsigned char *)(0x11400000 + i));
#endif
			*(unsigned char *)(0x11400000 + i) = 0x00;
#if DEBUG
			gecko_printf("(ERASED to %02x)\n", *(unsigned char *)(0x11400000 + i));
#endif
		}

		if (*(unsigned char *)(0x11400000 + i) != 0x00)
			gecko_printf("@ %08x: Couldn't erase byte %02x (IGNORING)\n", 0x11400000 + i, *(unsigned char *)(0x11400000 + i));
	}

	gecko_printf("Memory is done!\n");
	gecko_printf("\n");

	gecko_printf("Jumping to entry point %08x...\n", 0x00028710);

	//
	// Load the address of the BootMii app entry point in MEM1 into register 1
	//
	asm volatile("ldr r1,=0x28710");

	//
	// Jump to the BootMii app entry point
	//
	asm volatile("bx r1");

	//
	// This should never be reached...
	//
	while (1);

	return 0;
}

