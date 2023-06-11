/*
Copyright (C) 2023		nitr8

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/


#include "string.h"
#include "gecko.h"
#include "preload.h"


#define BUILDSTR		__DATE__" "__TIME__

#define MEM1_TARGET_HEAD	0x11400000


void *_main(void)
{
	//
	// Initialize the USB Gecko
	//
	gecko_init();

	gecko_printf("\n");
	gecko_printf("USB Gecko initialized\n");
	gecko_printf("\n");
	gecko_printf("============= PRELOADER =============\n");
	gecko_printf("\n");
	gecko_printf("BootMii loader for NANDBoot v0.5 (c) nitr8\n");
	gecko_printf("Built: %s\n", BUILDSTR);
	gecko_printf("Compiler: GCC v%d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
	gecko_printf("\n");

	gecko_printf("Copying main loader to %08x and jumping to entry point...\n", MEM1_TARGET_HEAD);

	memcpy((void *)MEM1_TARGET_HEAD, (void *)preload, preload_length);

	return (void *)MEM1_TARGET_HEAD;
}

