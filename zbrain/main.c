/** \file main.c
 *
 * Main routines for bfloader backend
 *
 * based on VDSP compatible backends provided by
 *      (c) Analog Devices, Inc. - 2001
 *
 * This supports the following boards:
 *
 * \li ZBrain-533
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
#define FLASH_BASE      0x20200000
// The buffer size in L1 memory (used for data transfer to flash)
#define BUFFER_SIZE		0x2000

// Page flipping access to flash: window size
#define PAGE_SIZE       0x100000
// Window mask address
#define PAGE_MASK       0x0fffff

struct sector_desc g_sectors[] = {
	{ 0x000000, 128, 0x8000 }
};

#define MAPSIZE (sizeof(g_sectors) / sizeof(struct sector_desc))

unsigned short g_mapsize = MAPSIZE;
struct sector_desc *g_sectormap = g_sectors;

////////////////////////////////////////////////////////////////////////////
// GLOBALS 
//

static
unsigned short g_buffer[BUFFER_SIZE >> 1];

//
// DATA NEEDED BY FRONTEND:

char           *AFP_Title           = "ZBrain Flash driver";
char           *AFP_Description     = "AM29LV641D";
long            AFP_Size            = BUFFER_SIZE;

extern long     AFP_SectorSize1;
extern short   *AFP_Buffer;


////////////////////////////////////////////////////////////////////////////
// LED toggling for 'busy' monitor

#define DOUT (*(unsigned short *) 0x201ff000)

void init_ledports()
{
	*pFIO_DIR    = 0x00f0;
	*pFIO_INEN   = 0x0f05;
	*pFIO_FLAG_D = 0x0000;
	*pFIO_INEN   = 0x0000; // enable input for buttons

	DOUT = 0x00;
}


void set_led(int leds)
{
	*pFIO_FLAG_D = (leds & 0xf) << 4;
	DOUT = (leds >> 4);
}

// Bank switching support for ZBrain
//

static short bank_no = -1;

static
void zbrain_pageselect(unsigned long addr)
{
	volatile unsigned char *base = (volatile unsigned char *) 0x200fff00;
	addr >>= (20 - 5); // Bank number: addr >> 20; shl by 5 for register val
	addr &= ~0x1f;
	if (bank_no == addr) return;

	bank_no = addr;
	base[0x36]   = 0xe0;    // upper nibble of port E is output
	base[0x34] = (base[0x34] & ~0xe0) | addr;
}

int flash_write(unsigned long offset, short value)
{
	zbrain_pageselect(offset);

	offset &= PAGE_MASK;
	offset += FLASH_BASE;
	asm("ssync;");
	*(volatile short *) offset = value;
	asm("ssync;");

	return ERR_NONE;
}

int flash_read(unsigned long offset, short *value)
{
	zbrain_pageselect(offset);
	offset &= PAGE_MASK;
	offset += FLASH_BASE;
	*value = *(volatile short *) offset;

	return ERR_NONE;
}

void ebiu_init()
{
	*pEBIU_AMBCTL0 = 0xf3c0ffc0;
	*pEBIU_AMBCTL1 = 0xffc2ffc0;
	*pEBIU_AMGCTL = 0xf;
}


int flash_init(int code)
{
	init_ledports();
	set_led(0);
	AFP_Buffer = g_buffer;

	if (code != 0x22d7) {
		return ERR_CHIP_UNSUPPORTED;
	}

	AFP_SectorSize1 = g_sectormap[0].size;

	return ERR_NONE;
}


