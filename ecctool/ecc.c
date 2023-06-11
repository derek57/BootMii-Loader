/*  Simple ECC verification code, originally by Segher */

#include <string.h>


#define ECC_OK 0
#define ECC_WRONG 1
#define ECC_INVALID 2
#define ECC_BLANK 3


static unsigned char parity(unsigned char x)
{
	unsigned char y = 0;

	while (x)
	{
		y ^= (x & 1);
		x >>= 1;
	}

	return y;
}

static void calc_ecc(unsigned char *data, unsigned char *ecc)
{
	unsigned char a[12][2];
	int i, j;
	unsigned int a0, a1;
	unsigned char x;

	memset(a, 0, sizeof(a));

	for (i = 0; i < 512; i++)
	{
		x = data[i];

		for (j = 0; j < 9; j++)
			a[3 + j][(i >> j) & 1] ^= x;
	}

	x = (a[3][0] ^ a[3][1]);
	a[0][0] = (x & 0x55);
	a[0][1] = (x & 0xaa);
	a[1][0] = (x & 0x33);
	a[1][1] = (x & 0xcc);
	a[2][0] = (x & 0x0f);
	a[2][1] = (x & 0xf0);

	for (j = 0; j < 12; j++)
	{
		a[j][0] = parity(a[j][0]);
		a[j][1] = parity(a[j][1]);
	}

	a0 = a1 = 0;

	for (j = 0; j < 12; j++)
	{
		a0 |= (a[j][0] << j);
		a1 |= (a[j][1] << j);
	}

	ecc[0] = a0;
	ecc[1] = (a0 >> 8);
	ecc[2] = a1;
	ecc[3] = (a1 >> 8);
}

unsigned char *calc_page_ecc(unsigned char *data)
{
	static unsigned char ecc[16];

	calc_ecc(data, ecc);
	calc_ecc(data + 512, ecc + 4);
	calc_ecc(data + 1024, ecc + 8);
	calc_ecc(data + 1536, ecc + 12);

	return ecc;
}

/*
int check_ecc(unsigned char *page)
{
	unsigned char *stored_ecc = page + 2048 + 48;

	if (page[2048] != 0xFF)
		return ECC_INVALID;

	if ((stored_ecc[0] == 0xFF) && (stored_ecc[1] == 0xFF))
		return ECC_BLANK;
	
	if (memcmp(stored_ecc, calc_page_ecc(page), 16))
		return ECC_WRONG;

	return ECC_OK;
}
*/

