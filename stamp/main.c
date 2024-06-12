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
// - DB : A is boot block (bottom)  (DevCode 0x22cb)
//
// - DT : B is boot block (bottom)  (not supported here yet, DevCode 0x22ca)
//
// The flash map base address in Blackfin memory space:
#define FLASH_BASE      0x20000000
// The buffer size in L1 memory (used for data transfer to flash)
#define BUFFER_SIZE		0x2000
// Number of sectors on flash
#define NUM_SECTORS_A   4
#define NUM_SECTORS_B   63
#define	NUM_SECTORS 	(NUM_SECTORS_A + NUM_SECTORS_B)
// Sector size
#define SECTOR_SIZE_A   0x02000
#define SECTOR_SIZE_B   0x10000

#define OFFSET_B        0x10000

#define FLASH_SIZE      0x400000

////////////////////////////////////////////////////////////////////////////
// GLOBALS 
//

static
unsigned short g_buffer[BUFFER_SIZE >> 1];

// DATA NEEDED BY FRONTEND:

char           *AFP_Title           = "STAMP BF537 Flash driver";
char           *AFP_Description     = "M29D320DB";
ProgCmd         AFP_Command         = NO_COMMAND;
int             AFP_ManCode         = -1;
int             AFP_DevCode         = -1;
unsigned long   AFP_Offset          = 0x0;
short          *AFP_Buffer;
long            AFP_Size            = BUFFER_SIZE;
long            AFP_Count           = -1;
long            AFP_Stride          = 1;
int             AFP_NumSectors      = NUM_SECTORS;
long            AFP_SectorSize1     = SECTOR_SIZE_A;
long            AFP_SectorSize2     = SECTOR_SIZE_B;
int             AFP_Sector          = -1;
int             AFP_Error           = 0;            // error code
int             AFP_Verify          = 0;        // verify flag
unsigned long   AFP_StartOff        = 0x0;          // sector start offset
unsigned long   AFP_EndOff          = 0x0;          // sector end offset
int             AFP_FlashWidth      = 0x10;         // width of the flash
int            *AFP_SectorInfo;

SectorLocation SectorInfo[NUM_SECTORS];

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

int flash_write(unsigned long offset, int value)
{
	offset += FLASH_BASE;
	asm("ssync;");
	*(unsigned volatile short *) offset = value;
	asm("ssync;");

	return NO_ERR;
}

int flash_read(unsigned long offset, short *value)
{
	offset += FLASH_BASE;
	*value = *(unsigned volatile short *) offset;
	return NO_ERR;
}

int flash_init()
{
	*pEBIU_AMGCTL  = 0xff;
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

	// 'odd' layout represented:
	if (block >= NUM_SECTORS_A) {
		sector = OFFSET_B  + (block - NUM_SECTORS_A) * AFP_SectorSize2;
	} else if (block == 0) {
		sector = 0x00000;
	} else if (block == 3) {
		sector = 0x8000;
	} else {
		// Block 1 and 2 follow at 0x4000
		sector = 0x2000 + block * 0x2000;
	}

	// send the erase block command to the flash
	
	return flash_sendcmd_blkerase(sector);
}


int flash_get_sectno(unsigned long offset, int *secno)
{
	int s = 0;

	if (offset >= FLASH_SIZE) return INVALID_SECTOR;

	if (offset > OFFSET_B) {
		s = (offset / AFP_SectorSize2) + (NUM_SECTORS_A - 1);
	} else {
		if       (offset < 0x4000) s = 0;
		else if (offset >= 0x8000) s = 3;
		else                       s = (offset - 0x2000) / 0x2000;
	}

	*secno = s;

	return NO_ERR;
}

int flash_get_sectaddr( unsigned long *start, unsigned long *end, int sector )
{
	if (sector >= NUM_SECTORS_A) {
		*start = OFFSET_B  + (sector - NUM_SECTORS_A) * AFP_SectorSize2;
		*end = SECTOR_SIZE_B;
	} else if (sector == 0) {
		*start = 0x0000;
		*end   = 0x4000;
	} else if (sector == 3) {
		*start = 0x8000;
		*end   = 0x10000;
	} else {
		*start = 0x4000 + sector * 0x2000;
		*end   = *start + 0x2000;
	}

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


