/*
Copyright (C) 2023		nitr8

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/


#include "gecko.h"
#include "crypto.h"
#include "nand.h"
#include "memory.h"
#include "utils.h"
#include "gpio.h"
#include "hollywood.h"
#include "string.h"
#include "start.h"
#include "ff.h"
#include "sdhc.h"
#include "armboot.h"
#include "freemem.h"


#define DEBUG			0
#define DEVEL			0
#define KEYS			0

#define BUILDSTR		__DATE__" "__TIME__

#define UNUSED(x)		((x) = (x))

#define DELAY			0x00000100

#define MEM1_TARGET_HEAD	0x10400000
#define MEM1_TARGET		0x00028000
#define MEM1_TARGET_BIN		(MEM1_TARGET + 0x700)
#define MEM1_TITLE_KEY		0x000B0000

#define BOOTMII_NAND_OFFSET	0x00063000
#define BOOTMII_NAND_START_PAGE	(BOOTMII_NAND_OFFSET / (PAGE_SIZE + PAGE_SPARE_SIZE))
#define BOOTMII_HEADER_SIZE	0x00000010

#define CONTENT_SIZE_OFFSET	0x000001D0
#define TITLE_KEY_OFFSET	0x000001BF
#define TITLE_IV_OFFSET		0x000001DC

#define ARMBOOT_FILE		"/bootmii/armboot.bin"


//
// ECC buffer (MUST be 128-bit aligned!!!)
//
static unsigned char ecc[PAGE_SPARE_SIZE] __attribute__((aligned(128)));

static unsigned char aes_key_old[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static unsigned char aes_iv_old[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


static void do_crypto(unsigned int nand_src, void *mem_source, void *mem_dest, int blocks, int iv_reset)
{
	debug_output(nand_src);
#if DEBUG
	gecko_printf("Reading %d bytes of page %03d from NAND into %08x...\n", blocks * 16, nand_src, (unsigned int)mem_source);
#endif
	//
	// Copy encrypted data from NAND page 193 to offset 0x00028000 in MEM1
	// NOTE: Starlet copies data without ECC for decryption
	//
	nand_read_page(nand_src, mem_source, &ecc);
#if DEVEL
	hexdump(mem_source, blocks * 16);
#endif
	//
	// Give it some time
	//
	udelay(DELAY);
#if DEBUG
	if ((blocks * 16) < PAGE_SIZE)
		gecko_printf("Decrypting %d bytes of NAND page %d from 0x%08x into %08x...\n", blocks * 16, nand_src, (unsigned int)mem_source, (unsigned int)mem_dest);
	else
		gecko_printf("Decrypting page %03d from %08x into %08x...\n", nand_src, (unsigned int)mem_source, (unsigned int)mem_dest);
#endif
	//
	// Decrypt 16 "blocks" (256 bytes in total) from offset 0x700 in NAND page 193 to dest. 0x00028700 in MEM1
	//
	// At the same time, reset the AES IV
	//
	aes_decrypt(mem_dest, mem_dest, blocks, iv_reset);
#if DEVEL
	hexdump(mem_dest, blocks * 16);
#endif
	//
	// Give it some time
	//
	udelay(DELAY);
}

static void which_one(unsigned char *array_old, unsigned char *array_new, const char *name)
{
	int i = 0;
	int flag = 0;

	gecko_printf("Setting");
#if KEYS
	gecko_printf(" ");
#endif
	for (i = 0; i < 16; i++)
	{
#if KEYS
		gecko_printf("%02x", array_new[i]);
#endif
		if (array_old[i] != array_new[i])
			flag = 1;
	}
#if KEYS
	gecko_printf(" as");
#endif
	if (flag)
		gecko_printf(" new ");

	gecko_printf("AES %s...\n", name);
}

static void aes_prepare(unsigned char *key, unsigned char *iv, int prepare)
{
	int i;

	//
	// Setup the crypto module
	//
	gecko_printf("Resetting AES module...\n");

	//
	// Reset the AES module
	//
	aes_reset();

	gecko_printf("Emptying AES IV...\n");

	if (!prepare)
		return;

	//
	// Empty the AES initialization vector
	//
	aes_empty_iv();

	which_one(aes_key_old, key, "key");

	//
	// Set the AES decryption key
	//
	aes_set_key(key);

	which_one(aes_iv_old, iv, "IV");

	//
	// Set the AES decryption initialization vector
	//
	aes_set_iv(iv);

	for (i = 0; i < 16; i++)
	{
		aes_key_old[i] = key[i];
		aes_iv_old[i] = iv[i];
	}
}

static int roundDown(int numToRound, int multiple)
{
	int remainder;
	int roundDown;

	if (multiple == 0)
		return numToRound;

	remainder = numToRound % multiple;

	if (remainder == 0)
		return numToRound;

	roundDown = ((int)(numToRound) / multiple) * multiple;

	return roundDown;
}

static int roundUp(int numToRound, int multiple)
{
	int remainder;

	if (multiple == 0)
		return numToRound;

	remainder = numToRound % multiple;

	if (remainder == 0)
		return numToRound;

	return numToRound + multiple - remainder;
}

void *_main(void)
{
	int i, ticket_offset, tmd_offset;
	int content_size, remaining_pages, remaining_data, remaining_content;

	typedef struct
	{
		u32 header_size;
		u16 type;
		u16 enc_data_start;
		u32 cert_size;
		//u32 reserved;
		u32 ticket_size;
		u32 tmd_size;
		//u32 enc_content_size;
		//u32 footer_size;
		u32 unused[3];
	} wad_t;

	//
	// Make compiler stop complaining about "variable not initialized"
	//
	wad_t *wad = NULL;

	static unsigned char bootmii_aes_key[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static unsigned char bootmii_aes_iv[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static unsigned char title_iv[16] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	FIL fd;
	FATFS fatfs;
	FRESULT fres;
	u32 bytes_written;
	int ret;

	//
	// Initialize the USB Gecko
	//
	gecko_init();

	gecko_printf("\n");
	gecko_printf("\n");
	gecko_printf("\n");
	gecko_printf("USB Gecko initialized\n");
	gecko_printf("\n");
	gecko_printf("============= BOOTMII LOADER =============\n");
	gecko_printf("\n");

	gecko_printf("Trying to mount the SD-Card...\n");

	//
	// Initialize the SD Host Controller
	//
	ret = sdhc_init();

	if (ret == 0)
	{
		//
		// Mount the SD-Card
		//
		fres = f_mount(0, &fatfs);

		if (fres != FR_OK)
			gecko_printf("[ERROR]: Unable to mount SD-Card (%d)\n", fres);
		else
		{
			gecko_printf("Trying to open SD:%s for writing...\n", ARMBOOT_FILE);

			//
			// Open the file for writing
			//
			fres = f_open(&fd, ARMBOOT_FILE, FA_CREATE_ALWAYS | FA_WRITE);

			if (fres != FR_OK)
				gecko_printf("[ERROR]: Cannot open file SD:%s (%d)\n", ARMBOOT_FILE, fres);
			else
			{
				gecko_printf("Trying to write new ARMBOOT binary to SD:%s...\n", ARMBOOT_FILE);

				//
				// Write the data
				//
				fres = f_write(&fd, (const void *)armboot, armboot_length, &bytes_written);

				if (fres != FR_OK)
					gecko_printf("[ERROR]: Cannot write file SD:%s (%d)\n", ARMBOOT_FILE, fres);
				else
				{
					gecko_printf("%d bytes successfully written to file SD:%s\n", bytes_written, ARMBOOT_FILE);

					//
					// If successful, close the file
					//
					f_close(&fd);
				}
			}
		}
	}

	//
	// Initialize the crypto module
	//
	crypto_initialize();

	gecko_printf("Crypto initialized\n");

	//
	// Initialize the NAND
	//
	nand_initialize();

	gecko_printf("NAND initialized\n");

	//
	// Read the first 2 pages of BootMii within NAND into MEM1
	//
	for (i = 0; i < 2; i++)
	{
		debug_output(BOOTMII_NAND_START_PAGE + i);
		gecko_printf("Reading page %03d from NAND into %08x...\n", BOOTMII_NAND_START_PAGE + i, MEM1_TARGET + (i * PAGE_SIZE));

		//
		// Copy encrypted data from NAND pages 192 & 193 to offset 0x00028000 in MEM1
		// NOTE: Starlet copies data without ECC for decryption
		//
		nand_read_page(BOOTMII_NAND_START_PAGE + i, (void *)MEM1_TARGET + (i * PAGE_SIZE), &ecc);

		//
		// Give it some time
		//
		udelay(DELAY);
	}

	//
	// Clear the WAD header struct
	//
	memset((void *)wad, 0, sizeof(wad_t));

	//
	// Copy data from offset 0x00028000 in MEM1 into the WAD header struct
	//
	memcpy((void *)wad, (void *)MEM1_TARGET, sizeof(wad_t));
#if DEVEL
	hexdump((void *)wad, sizeof(wad_t));
#endif
	//
	// Set the start address of the ticket
	//
	ticket_offset = (wad->cert_size + sizeof(wad_t));

	gecko_printf("Reading ticket from %08x\n", (MEM1_TARGET + ticket_offset));

	//
	// Set the start address of the title metadata
	//
	tmd_offset = (ticket_offset + wad->ticket_size + sizeof(wad_t));

	gecko_printf("Reading tmd from %08x\n", (MEM1_TARGET + tmd_offset));

	//
	// Setup the title AES IV
	//
	memcpy((void *)title_iv, (void *)(MEM1_TARGET + ticket_offset + TITLE_IV_OFFSET), 16);
#if DEVEL
	hexdump((void *)title_iv, 16);
#endif
	//
	// Setup the title AES Key
	//
	memcpy((void *)MEM1_TITLE_KEY, (void *)(MEM1_TARGET + ticket_offset + TITLE_KEY_OFFSET), 16);
#if DEVEL
	hexdump((void *)MEM1_TITLE_KEY, 16);
#endif
	//
	// Set the AES decryption initialization vector
	//
	// Note: BootMii's Title AES IV usually reads
	// 00000001000000010000000400000000
	// according to the BootMii installation within NAND
	//
	// For some reason, before the Title AES Key is
	// being decrypted, the AES IV becomes this instead:
	// 00000001000000010000000000000000
	//
	// [STARLET]: AES: key=ebe42a225e8593e448d9c5457381aaf7 98000000
	// [STARLET]: AES: iv=00000001000000010000000000000000
	// [STARLET]: AES: src_ptr=000b0000, dst_ptr=000b0000, iv_reset=1, num_bytes=16
	//

	//
	// NASTY: This really fixes the problem mentioned above
	//
	if ((title_iv[11] != 0x00) && (title_iv[11] == 0x04))
	{
		gecko_printf("\n");
		gecko_printf("[WARNING]: Title AES IV has been corrupted!\n");
		gecko_printf("[WARNING]: Array element %d reads %02x while it should read %02x!\n", 11, title_iv[11], 0x00);
		gecko_printf("[WARNING]: Will correct that and continue...!\n");
		gecko_printf("\n");
#if DEVEL
		gecko_printf("BEFORE: @");
		hexdump((void *)title_iv, 16);
		gecko_printf("\n");
		gecko_printf("\n");
#endif
		title_iv[11] = 0x00;
#if DEVEL
		gecko_printf("AFTER:  @");
		hexdump((void *)title_iv, 16);
		gecko_printf("\n");
		gecko_printf("\n");
#endif
		gecko_printf("Setting CORRECTED title AES IV...\n");
	}
	else
		gecko_printf("Setting title AES IV...\n");
#if DEVEL
	hexdump((void *)title_iv, 16);
#endif
	//
	// Preconfigure the AES module
	//
	aes_prepare(otp.common_key, title_iv, 1);

	gecko_printf("Decrypting title key...\n");

	//
	// Decrypt the title AES key using the common key in OTP and title AES IV
	//
	// At the same time, reset the AES IV
	//
	aes_decrypt((void *)MEM1_TITLE_KEY, (void *)MEM1_TITLE_KEY, 1, 0);

	//
	// Give it some time
	//
	udelay(DELAY);
#if DEVEL
	hexdump((void *)MEM1_TITLE_KEY, 16);
#endif
	//
	// Copy the decrypted AES key into the array for decryption of the content
	//
	memcpy((void *)bootmii_aes_key, (void *)MEM1_TITLE_KEY, 16);

	content_size = *(unsigned int *)(MEM1_TARGET + tmd_offset + CONTENT_SIZE_OFFSET);
	remaining_content = (content_size - 0x100);
	remaining_pages = roundDown(remaining_content, PAGE_SIZE);
	remaining_data = roundUp(remaining_content - remaining_pages, 0x10);
#if DEBUG
	gecko_printf("\n");
	gecko_printf("Total content size = %08x bytes\n", content_size);
	gecko_printf("Remaining content size = %08x bytes\n", remaining_content);
	gecko_printf("Remaining pages = %d\n", remaining_pages / PAGE_SIZE);
	gecko_printf("Remaining data = %d bytes\n", remaining_data);
#endif
	gecko_printf("\n");

	//
	// Preconfigure the AES module
	//
	aes_prepare(bootmii_aes_key, bootmii_aes_iv, 1);
#if DEVEL
	hexdump((void *)bootmii_aes_iv, 16);
#endif
	gecko_printf("Decrypting content...\n");

	//
	// Go for the remaining binary data within the first page
	//
	do_crypto(BOOTMII_NAND_START_PAGE + 1, (void *)MEM1_TARGET, (void *)MEM1_TARGET_BIN, 16, 0);

	//
	// Now go for the data within all the pages
	//
	for (i = 0; i < (remaining_pages / PAGE_SIZE); i++)
		do_crypto((BOOTMII_NAND_START_PAGE + 2) + i, (void *)MEM1_TARGET_BIN + 0x100 + (i * PAGE_SIZE), (void *)MEM1_TARGET_BIN + 0x100 + (i * PAGE_SIZE), 128, 1);

	//
	// This is for the remaining binary data within the last page
	//
	do_crypto((BOOTMII_NAND_START_PAGE + 2) + i, (void *)MEM1_TARGET_BIN + 0x100 + (i * PAGE_SIZE), (void *)MEM1_TARGET_BIN + 0x100 + (i * PAGE_SIZE), remaining_data / 16, 1);

	gecko_printf("\n");
	gecko_printf("Cleanup...\n");

	//
	// Reset the AES module
	//
	aes_prepare(0, 0, 0);

	gecko_printf("\n");
	gecko_printf("Loading entry point %08x for freeing memory...\n", MEM1_TARGET_HEAD);
	gecko_printf("\n");

	//
	// Prepare for freeing memory
	//
	memcpy((void *)MEM1_TARGET_HEAD, (void *)freemem, freemem_length);

	return (void *)MEM1_TARGET_HEAD;
}

