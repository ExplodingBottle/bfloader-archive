/** \file main.c
 *
 * Main routines for bfloader backend
 *
 * based on VDSP compatible backends provided by
 *      (c) Analog Devices, Inc. - 2001
 *
 * This supports the following boards:
 *
 * \li STAMP-BF533
 * \li STAMP-BF537
 * \li EZKIT-BF537
 *
 * Modified and bug fixed for bfloader
 * Also removed unreadable hungarian variable notation
 * 16 bit Flash chips only!
 *    Martin Strubel <hackfin@section5.ch>
 *
 * This code is provided 'as is' without warranty. Use on your
 * own risk.
 *
 *
 */


#include "error.h"
#include "bfloader.h"
#include "flash.h"
#include <cdefBF533.h>

////////////////////////////////////////////////////////////////////////////
// These are the values customized to your flash:
//
// Manufacturer Code: 0x0020
//
// The M29W320 has two banks A and B
// - DB : A is boot block (bottom)
// - DT : B is boot block (bottom)  
//
// The flash map base address in Blackfin memory space:
#define FLASH_BASE      0x20000000
// The buffer size in L1 memory (used for data transfer to flash)
#define BUFFER_SIZE		0x2000

// Top device sector map

struct sector_desc g_sectormap_top[] = {
	{ 0x000000, 63, 0x10000 },
	{ 0x3f0000,  1, 0x08000 },
	{ 0x3f8000,  2, 0x02000 },
	{ 0x3fc000,  1, 0x04000 }
};

// Bottom device sector map

struct sector_desc g_sectormap_bot[] = {
	{ 0x000000,  1, 0x04000 },
	{ 0x004000,  2, 0x02000 },
	{ 0x008000,  1, 0x08000 },
	{ 0x010000, 63, 0x10000 }
};

#define NUM_SECTORS 35

#define MAPSIZE (sizeof(g_sectormap_top) / sizeof(struct sector_desc))

////////////////////////////////////////////////////////////////////////////
// GLOBALS 
//

unsigned short g_mapsize = MAPSIZE;
struct sector_desc *g_sectormap = 0;

static
unsigned short g_buffer[BUFFER_SIZE >> 1];

// DATA NEEDED BY FRONTEND:

char           *AFP_Title           = "STAMP BF537 Flash driver";
char           *AFP_Description     = "M29D320DB";
long            AFP_Size            = BUFFER_SIZE;

extern long     AFP_SectorSize1;
extern short   *AFP_Buffer;

////////////////////////////////////////////////////////////////////////////

// LED monitor auxiliaries
void init_ledports()
{
	*pFIO_DIR    = 0x001c;
	*pFIO_FLAG_D = 0x0000;
	*pFIO_INEN   = 0x0160; // enable input for buttons
}

void set_led(int leds)
{
	*pFIO_FLAG_D = (~leds & 7) << 2;
}

int flash_write(unsigned long offset, short value)
{
	offset += FLASH_BASE;
	asm("ssync;");
	*(volatile short *) offset = value;
	asm("ssync;");

	return ERR_NONE;
}

int flash_read(unsigned long offset, short *value)
{
	offset += FLASH_BASE;
	*value = *(volatile short *) offset;
	return ERR_NONE;
}

void ebiu_init()
{
	*pEBIU_AMGCTL  = 0xff;
	*pEBIU_AMBCTL0 = 0x7bb07bb0;
	*pEBIU_AMBCTL1 = 0x7bb07bb0;
}

int flash_init(int code)
{
	init_ledports();
	set_led(0);

	AFP_Buffer = g_buffer;

	switch (code) {
		case 0x22ca: // Top device
			g_sectormap = g_sectormap_top;
			AFP_SectorSize1 = g_sectormap[0].size;
			break;
		case 0x22cb:
			g_sectormap = g_sectormap_bot;
			AFP_SectorSize1 = g_sectormap[3].size;
			break;
		default:
			return ERR_CHIP_UNSUPPORTED;
	}

	return ERR_NONE;
}


