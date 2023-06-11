/*
Copyright (C) 2023		nitr8

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/


#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "ecc.h"


#define BLOCK_SIZE	0x21000
#define DATA_SIZE	0x800
#define SPARE_SIZE	0x40
#define ECC_SIZE	0x10


static unsigned int file_size(FILE *infile)
{
	unsigned int filesize;

	fseek(infile, 0, 2);
	filesize = ftell(infile);
	fseek(infile, 0, 0);

	return filesize;
}

int main(int argc, char *argv[])
{
	if (argc == 3)
	{
		unsigned int i, v4, bytes, filesize;
		int pagesize = 0;
		FILE *infile = fopen(argv[1], "rb");
		FILE *outfile = fopen(argv[2], "wb");
		void *buffer = malloc(DATA_SIZE);
		void *spare = malloc(DATA_SIZE);

		memset(spare, 0, DATA_SIZE);
		filesize = file_size(infile);
		//printf("filesize: %d", filesize);

		if (filesize % 0x20000 <= 0)
			v4 = filesize / 0x20000;
		else
			v4 = filesize / 0x20000 + 1;

		while (1)
		{
			bytes = fread(buffer, 1, DATA_SIZE, infile);

			if (bytes > 0)
			{
				if (bytes > (DATA_SIZE - 1))
				{
					fwrite(buffer, 1, DATA_SIZE, outfile);
				}
				else
				{
					fwrite(buffer, 1, bytes, outfile);
					fwrite(spare, 1, DATA_SIZE - bytes, outfile);
				}

				fputc(0xFF, outfile);
				fwrite(spare, 1, 0x2F, outfile);
				fwrite(calc_page_ecc(buffer), 1, ECC_SIZE, outfile);
				pagesize += (DATA_SIZE + SPARE_SIZE);
			}

			if (bytes <= (DATA_SIZE - 1))
				break;

			memset(buffer, 0, DATA_SIZE);
		}

		for (i = 0; i < BLOCK_SIZE * v4 - pagesize; ++i)
			fputc(0, outfile);

		fclose(infile);
		fclose(outfile);
		putchar(0xA);

		if (buffer)
			free(buffer);

		if (spare)
			free(spare);

		return 0;
	}
	else
	{
		printf("Usage: %s input output\n", *argv);
		return 1;
	}
}

