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
// Number of sectors on flash
#define	NUM_SECTORS 	128
// Sector size
#define SECTOR_SIZE     0x8000

// Page flipping access to flash: window size
#define PAGE_SIZE       0x100000
// Window mask address
#define PAGE_MASK       0x0fffff

////////////////////////////////////////////////////////////////////////////
// GLOBALS 
//

static
unsigned short g_buffer[BUFFER_SIZE >> 1];
SectorLocation SectorInfo[NUM_SECTORS];

//
// DATA NEEDED BY FRONTEND:

char           *AFP_Title           = "ZBrain Flash driver";
char           *AFP_Description     = "AM29LV641D";
ProgCmd         AFP_Command         = NO_COMMAND;
int             AFP_ManCode         = -1;
int             AFP_DevCode         = -1;
unsigned long   AFP_Offset          = 0x0;
short          *AFP_Buffer;
long            AFP_Size            = BUFFER_SIZE;
long            AFP_Count           = -1;
long            AFP_Stride          = 1;
int             AFP_NumSectors      = NUM_SECTORS;
long            AFP_SectorSize1     = SECTOR_SIZE;
int             AFP_Sector          = -1;
int             AFP_Error           = 0;            // error code
int             AFP_Verify          = 0;            // verify flag
unsigned long   AFP_StartOff        = 0x0;          // sector start offset
unsigned long   AFP_EndOff          = 0x0;          // sector end offset
int             AFP_FlashWidth      = 0x10;         // width of the flash
int            *AFP_SectorInfo;


////////////////////////////////////////////////////////////////////////////
// LED toggling for 'busy' monitor

#define DOUT (*(unsigned short *) 0x201ff000)

void init_ledports()
{
	*pFIO_DIR    = 0x00f0;
	*pFIO_FLAG_D = 0x00a0;
	*pFIO_INEN   = 0x0000; // enable input for buttons

	DOUT = 0x00;
}


void set_led(int leds)
{
	*pFIO_FLAG_D = (leds & 0xf) << 4;
	DOUT = (leds >> 4) & 0xf;
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

int flash_write(unsigned long offset, int value)
{
	zbrain_pageselect(offset);

	offset &= PAGE_MASK;
	offset += FLASH_BASE;
	asm("ssync;");
	*(unsigned volatile short *) offset = value;
	asm("ssync;");

	return NO_ERR;
}

int flash_read(unsigned long offset, short *value)
{
	zbrain_pageselect(offset);
	offset &= PAGE_MASK;
	offset += FLASH_BASE;
	*value = *(unsigned volatile short *) offset;

	return NO_ERR;
}

int flash_init()
{
	*pEBIU_AMGCTL = 0xff;
	*pEBIU_AMBCTL0 = 0x7bb07bb0;
	*pEBIU_AMBCTL1 = 0x7bb07bb0;
	init_ledports();
	set_led(0);

	return NO_ERR;
}

int flash_erase()
{
	return flash_sendcmd_fullerase();
}

int flash_erase_blk(int block)
{
	unsigned long sector;

	// if the block is invalid just return
	if ((block < 0) || (block >= AFP_NumSectors))
		return INVALID_BLOCK;

	sector = block * AFP_SectorSize1;

	return flash_sendcmd_blkerase(sector);
}


int flash_get_sectno(unsigned long offset, int *secno)
{
	int s = 0;

	s = (offset / AFP_SectorSize1);

	if (s >= 0 && s < AFP_NumSectors)
		*secno = s;
	else
		return INVALID_SECTOR;

	return NO_ERR;
}


int flash_get_sectaddr(unsigned long *start, unsigned long *end, int sector)
{
	// main block
	if( sector < NUM_SECTORS ) {
		*start = sector * AFP_SectorSize1;
		*end = ( (*start) + AFP_SectorSize1 );
	}
	else
		return INVALID_SECTOR;


	*end -= 1;

	return NO_ERR;
}



////////////////////////////////////////////////////////////////////////////
// MAIN

int main()
{
	int i = 0;

	AFP_Buffer = g_buffer;

	//initiate sector information structures
	for(i = 0; i < AFP_NumSectors; i++) {
		flash_get_sectaddr(&SectorInfo[i].start,
				&SectorInfo[i].end, i);
	}

	AFP_SectorInfo = (int *) &SectorInfo[0];

	flash_init();
	flash_get_codes();

	// main loop
	while (1) {
		// the plug-in will set a breakpoint at "AFP_BreakReady" so it knows
		// when we are ready for a new command because the DSP will halt
		//
		// the jump is used so that the label will be part of the debug
		// information in the driver image otherwise it may be left out
		// since the label is not referenced anywhere
		asm("AFP_BreakReady:");
		asm("nop;");
		if (0)
			asm("jump AFP_BreakReady;");

		switch (AFP_Command)
		{
			case GET_CODES:
				AFP_Error = flash_get_codes();
				break;
			case RESET:
				AFP_Error = flash_reset();
				break;
			case WRITE:
				AFP_Error = flash_writechunk(AFP_Offset, AFP_Count,
						AFP_Stride, AFP_Buffer );
				break;
			case FILL:
				AFP_Error = flash_fill(AFP_Offset, AFP_Count,
						AFP_Stride, AFP_Buffer );
				break;
			case ERASE_ALL:
				AFP_Error = flash_erase();
				break;
			case ERASE_SECT:
				AFP_Error = flash_erase_blk(AFP_Sector);
				break;
			case READ:
				AFP_Error = flash_readchunk(AFP_Offset, AFP_Count,
						AFP_Stride, AFP_Buffer);
				break;
			case GET_SECTNUM:
				AFP_Error = flash_get_sectno(AFP_Offset, &AFP_Sector);
			case GET_SECSTARTEND:
				AFP_Error = flash_get_sectaddr(&AFP_StartOff,
						&AFP_EndOff, AFP_Sector);
				break;
			case NO_COMMAND:
			default:
				AFP_Error = UNKNOWN_COMMAND;
				break;
		}
		AFP_Command = NO_COMMAND;
	}
	return 0;
}


