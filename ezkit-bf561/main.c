/** \file main.c
 *
 * Main routines for bfloader backend
 *
 * based on VDSP compatible backends provided by
 *      (c) Analog Devices, Inc. - 2001
 *
 * This supports the following boards:
 *
 * \li EZKIT-BF561
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
//
// The flash map base address in Blackfin memory space:
#define FLASH_BASE      0x20000000
// The buffer size in L1 memory (used for data transfer to flash)
#define BUFFER_SIZE		0x2000

// Flash sector map of the M29W640D[T,B]
//
// Top device (DevCode 0x22de) sector map


struct sector_desc g_sectormap_top[] = {
	{ 0x000000, 127, 0x10000 },
	{ 0x3f0000,   8, 0x02000 },
};

// Bottom device (DevCode 0x22df) sector map


struct sector_desc g_sectormap_bot[] = {
	{ 0x000000,   8, 0x02000 },
	{ 0x010000, 127, 0x10000 },
};

#define NUM_SECTORS 135

// We can safely do this, since both maps have the same size
#define MAPSIZE (sizeof(g_sectormap_top) / sizeof(struct sector_desc))

////////////////////////////////////////////////////////////////////////////
// GLOBALS 
//

unsigned short g_mapsize = MAPSIZE;
struct sector_desc *g_sectormap = 0;

static
short  g_buffer[BUFFER_SIZE >> 1];
SectorLocation SectorInfo[NUM_SECTORS];

//
// DATA NEEDED BY FRONTEND:

char           *AFP_Title           = "EZKIT-BF561 Flash driver";
char           *AFP_Description     = "M29W640D[B,T]";
long            AFP_Size            = BUFFER_SIZE;

extern long     AFP_SectorSize1;
extern short   *AFP_Buffer;

////////////////////////////////////////////////////////////////////////////

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

void ebiu_init(void)
{
	*pEBIU_AMGCTL  = 0xff;
	*pEBIU_AMBCTL0 = 0x7bb07bb0;
	*pEBIU_AMBCTL1 = 0x7bb07bb0;
}


int flash_init(int code)
{
	// Calc number of sectors from Map:
	
	switch (code) {
		case 0x22de: // Top device
			g_sectormap = g_sectormap_top;
			AFP_SectorSize1 = g_sectormap_top[0].size;
			break;
		case 0x22df:
			g_sectormap = g_sectormap_bot;
			AFP_SectorSize1 = g_sectormap_bot[MAPSIZE - 1].size;
			break;
		default:
			return ERR_CHIP_UNSUPPORTED;
	}

	AFP_Buffer = g_buffer;

	return ERR_NONE;
}

