/** \file main.c
 *
 * Main routines for bfloader backend
 *
 * based on VDSP compatible backends provided by
 *      (c) Analog Devices, Inc. - 2001
 *
 * This supports the following boards from tinyboards.com:
 *
 * \li CM-BF533
 * \li CM-BF537
 *
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
// Number of sectors on flash
#define NUM_SECTORS     16

// Sector size
#define SECTOR_SIZE     0x20000

#define FLASH_SIZE      0x400000

#define LASTSECTOR_ADDR 0x1e0000

#define FLASH_TIMEOUT   0x800000


////////////////////////////////////////////////////////////////////////////
// GLOBALS 
//

static
unsigned short g_buffer[BUFFER_SIZE >> 1];

// DATA NEEDED BY FRONTEND:

char           *AFP_Title           = "CM-BF53X flash driver";
char           *AFP_Description     = "28F320J3";
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
unsigned long   AFP_FlashSize       = 0;            // calculated flash size
int            *AFP_SectorInfo;

SectorLocation SectorInfo[NUM_SECTORS];

////////////////////////////////////////////////////////////////////////////

int flash_reset(void)
{
	flash_write(0, ICMD_READARRAY);
	return ERR_NONE;
}

int flash_get_codes(void)
{
	short code;
	flash_write(0, ICMD_READID);

	flash_read(0, &code); AFP_ManCode = code;
	flash_read(2, &code); AFP_DevCode = code;

	// Get flash back to read mode
	flash_reset();

	return ERR_NONE;
}


int flash_write(unsigned long offset, short value)
{
	offset += FLASH_BASE;
	asm("ssync;");
	*(unsigned volatile short *) offset = value;
	asm("ssync;");

	return ERR_NONE;
}

int flash_poll_status(unsigned long offset)
{
	int count = 0;
	unsigned short status;
	int error = 0;

	do {
		flash_read(0, &status);
		count++;
	} while ( !(status & SR_READY)
			  && (count < FLASH_TIMEOUT));
	if (count == FLASH_TIMEOUT) error = ERR_POLL_TIMEOUT;
	return error;
}

int flash_unlock(unsigned long offset)
{
	int error;

	flash_write(0, ICMD_CLRSTATUS);
	flash_write(0, ICMD_LOCK);
	flash_write(0, ICMD_ERASECONF);

	error = flash_poll_status(offset);

	flash_reset();
	
	return error;
}

int flash_readchunk(unsigned long start, long count, long stride, short *data)
{
	long i = 0;
	unsigned long offset = start;
	int leftover = count % 2;

	stride <<= 1;

	if (count > AFP_Size) return ERR_BUFSIZE_EXCESS;

	for (i = 0; (i < count / 2); i++) {
		// Read flash
		flash_read(offset, &data[i]);
		offset += stride;
	}
	if (leftover) {
		flash_read( offset, &data[i]);
	}

	return ERR_NONE;
}

int flash_writechunk(unsigned long start, long count,
		long stride, short *data)
{
	long i = 0;
	unsigned long offset = start;
	int leftover = count % 2;		
	int error = ERR_NONE;
	short cmp = 0;
	int verify_err = 0;

	if (count > AFP_Size) return ERR_BUFSIZE_EXCESS;

	stride <<= 1;

	// Write words
	flash_unlock(offset);  // per entire chip
	for (i = 0; i < count / 2; i++) {
		flash_write(offset, ICMD_CLRSTATUS);
		flash_write(offset, ICMD_PROG_WORD);
		flash_write(offset, data[i]);
		error = flash_poll_status(offset);
		if (error != ERR_NONE) return error;
		if (AFP_Verify) {
			flash_write(0, ICMD_READARRAY);
			flash_read(offset, &cmp);
			if (cmp != data[i]) {
				verify_err = 1;
				break;
			}
		}
		offset += stride;
	}
	// Write remainder:
	if (leftover && error == ERR_NONE && !verify_err) {
		flash_write(offset, ICMD_CLRSTATUS);
		flash_write(offset, ICMD_PROG_WORD);
		flash_write(offset, data[i]);
		error = flash_poll_status(offset);
		if (AFP_Verify) {
			flash_write(0, ICMD_READARRAY);
			flash_read(offset, &cmp);
			if ((cmp & 0xff) != (data[i] & 0x00ff)) {
				verify_err = 1;
			}
		}
	}

	if (verify_err) error = ERR_VERIFY_WRITE;

	return error;
}

int flash_read(unsigned long offset, short *value)
{
	offset += FLASH_BASE;
	*value = *(unsigned volatile short *) offset;
	return ERR_NONE;
}

void ebiu_init()
{
	*pEBIU_AMGCTL  = 0xff;
	*pEBIU_AMBCTL0 = 0x7bb07bb0;
	*pEBIU_AMBCTL1 = 0x7bb07bb0;
}

int erase_sector(unsigned long addr)
{
	unsigned short status;
	unsigned long count = 0;
	int error = ERR_NONE;

	flash_write(addr, ICMD_CLRSTATUS);
	flash_write(addr, ICMD_BLKERASE);
	flash_write(addr, ICMD_ERASECONF);

	error = flash_poll_status(addr);
	if (error != ERR_NONE) return error;

	count = FLASH_TIMEOUT;
	do {
		flash_write(addr, ICMD_READARRAY);
		count--; 
 		flash_read(addr, &status);
	} while ((count) && status != 0xffff);
	if (!count) error = ERR_POLL_TIMEOUT;
	
	return error;
	
}

int flash_erase()
{
	unsigned long sector = 0x000000;
	int error;
	
	do
	{
		error = erase_sector(sector);
#ifdef DEBUG
		printf("  Status of block %xh: %xh\n", sector, error);
#endif
		sector += SECTOR_SIZE;
	}
	while ((sector <= LASTSECTOR_ADDR) && (error != ERR_POLL_TIMEOUT));
	return error;
}

int flash_erase_blk(int block)
{
	unsigned long sector;

	// if the block is invalid just return
	if ((block < 0) || (block >= AFP_NumSectors))
		return ERR_INVALID_BLOCK;

	sector = block * SECTOR_SIZE;

	// send the erase block command to the flash
	
	return erase_sector(sector);
}


int flash_get_sectno(unsigned long offset, int *secno)
{
	int s = 0;

	if (offset >= FLASH_SIZE) return ERR_INVALID_SECTOR;

	s = offset / AFP_SectorSize1;

	*secno = s;

	return ERR_NONE;
}

int flash_get_sectaddr( unsigned long *start, unsigned long *end, int sector )
{
	*start = sector * SECTOR_SIZE;
	*end   = *start + SECTOR_SIZE;

	return ERR_NONE;
}

int flash_init(int code)
{
	if (code != 0x16) {
		return ERR_CHIP_UNSUPPORTED;
	}
	return ERR_NONE;
}

////////////////////////////////////////////////////////////////////////////
// MAIN

int main()
{
	ebiu_init();
	flash_get_codes();
	AFP_Error = flash_init(AFP_DevCode);
	AFP_FlashSize = 2 * 0x100000;
	AFP_Buffer = g_buffer;

#if 0
	// Initialize sector information structures
	int i;
	for (i = 0; i < AFP_NumSectors; i++) {
		flash_get_sectaddr(&SectorInfo[i].start,
				&SectorInfo[i].end, i);
	}

	AFP_SectorInfo = (unsigned long *) SectorInfo;
#endif

	// main loop
	while (1) {
		// This is where the break is set from the flashloader.
		// The DSP will stop here and process the command set by the
		// flashloader

		asm("AFP_BreakReady:");
		asm("nop;");

		switch (AFP_Command)
		{
			case RESET:
				AFP_Error = flash_reset();
				break;
			case WRITE:
				AFP_Error = flash_writechunk(AFP_Offset, AFP_Count,
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
				AFP_Error = ERR_UNKNOWN_COMMAND;
				break;
		}
		AFP_Command = NO_COMMAND;
	}
	return 0;
}
