/**
 * Common routines for bfloader backend
 *
 * 12 / 2005 Martin Strubel <hackfin@section5.ch>
 *
 */

#include "error.h"
#include "bfloader.h"
#include "flash.h"

////////////////////////////////////////////////////////////////////////////
// GLOBALS
//

ProgCmd         AFP_Command         = NO_COMMAND;
int             AFP_ManCode         = -1;
int             AFP_DevCode         = -1;
unsigned long   AFP_Offset          = 0x0;
short          *AFP_Buffer;
long            AFP_Count           = -1;
long            AFP_Stride          = 1;
int             AFP_NumSectors      = 0;
long            AFP_SectorSize1     = 0;
int             AFP_Sector          = -1;
int             AFP_Error           = 0;            // error code
int             AFP_Verify          = 0;            // verify flag
unsigned long   AFP_StartOff        = 0x0;          // sector start offset
unsigned long   AFP_EndOff          = 0x0;          // sector end offset
int             AFP_FlashWidth      = 0x10;         // width of the flash
unsigned long   AFP_FlashSize       = 0;            // calculated flash size


extern long AFP_Size;

////////////////////////////////////////////////////////////////////////////

int flash_get_codes()
{
	FWORD code;
	// send the auto select command to the flash
	flash_write(WRITESEQ1, CMD_GETCODE1);
	flash_write(WRITESEQ2, CMD_GETCODE2);
	flash_write(WRITESEQ3, CMD_GETCODE3);


	// now we can read the codes
	flash_read(ADDR_DEV, &code);
	AFP_ManCode = code;

	flash_read(ADDR_ID, &code);
	AFP_DevCode = code;

	// Get flash back to read mode
	flash_reset();

	return ERR_NONE;
}


int flash_reset()
{
	// send the reset command to the flash
	flash_write(WRITESEQ1, CMD_RESET);

	// reset should be complete
	return ERR_NONE;
}

int flash_poll_toggle(unsigned long offset)
{
	int error = ERR_NONE;	// flag to indicate error
	FWORD a, b;

	while (error == ERR_NONE)
	{
		flash_read(offset, &a);
		flash_read(offset, &b);

		a ^= b;
		if( !(a & SR_TOGGLE))    // toggling ?
			break;

		if( !(b & SR_ERROR))     // contine when no error
			continue;
		else {
			flash_read(offset, &a);
			flash_read(offset, &b);
			a ^= b;
			if( !(a & SR_TOGGLE))
				break;
			else {
				error = ERR_POLL_TIMEOUT;
				flash_reset();
			}
		}
	}
	return error;
}


int flash_sendcmd_fullerase()
{
	int error = ERR_NONE;	// tells us if there was an error erasing flash

	flash_write(WRITESEQ1, WRITEDATA1);
	flash_write(WRITESEQ2, WRITEDATA2);
	flash_write(WRITESEQ3, WRITEDATA3);
	flash_write(WRITESEQ4, WRITEDATA4);
	flash_write(WRITESEQ5, WRITEDATA5);
	flash_write(WRITESEQ6, WRITEDATA6);

	// poll until the command has completed
	error = flash_poll_toggle(0x0000);

	// erase should be complete
	return error;
}

int flash_sendcmd_blkerase(unsigned long sector_addr)
{

	flash_write(WRITESEQ1, WRITEDATA1);
	flash_write(WRITESEQ2, WRITEDATA2);
	flash_write(WRITESEQ3, WRITEDATA3);
	flash_write(WRITESEQ4, WRITEDATA4);
	flash_write(WRITESEQ5, WRITEDATA5);
	flash_write(sector_addr, CMD_BLKERASE);

	return flash_poll_toggle(sector_addr);
}

int flash_unlock(unsigned long offset)
{
	offset &= 0xffff0000;
	// send the unlock command to the flash
	// ORed with lOffsetAddr so we know what block we are in
	flash_write((WRITESEQ1 | offset), CMD_UNLOCK1);
	flash_write((WRITESEQ2 | offset), CMD_UNLOCK2);
	flash_write((WRITESEQ3 | offset), CMD_UNLOCK3);

	return ERR_NONE;
}

int flash_fill(unsigned long start, long count, long stride, FWORD *data)
{
	long i = 0;
	unsigned long offset = start;
	int error = ERR_NONE;
	FWORD cmp = 0;
	int verify_err = 0;
	unsigned long new_offset = 0;
	int byteswap = 0;

	stride <<= 1;

	// if we have an odd offset we need to write a byte
	// to the first location and the last
	if (offset % 2 != 0) {
		// read the offset - 1 and OR in our value
		new_offset = offset - 1;

		flash_read(new_offset, &cmp);
		cmp &= 0x00ff;
		cmp |= (data[0] << 8) & 0xff00;

		// unlock the flash, do the write, and wait for completion
		flash_unlock(new_offset);
		flash_write(new_offset, cmp);
		error = flash_poll_toggle(new_offset);

		// move to the last offset
		new_offset = ((offset - 1) + (count * stride));

		// read the value and OR in our value
		flash_read(new_offset, &cmp);
		cmp &= 0xff00;
		cmp |= ( data[0] >> 8) & 0x00ff;

		// unlock the flash, do the write, and wait for completion
		flash_unlock(new_offset);
		flash_write(new_offset, cmp);
		error = flash_poll_toggle(new_offset);

		// increment the offset and count
		offset = (offset - 1) + stride;
		count--;

		byteswap = 1;
	}

	if( byteswap == 1 )
	{
		int nTemp = (char) data[0];
		data[0] &= 0xff00;
		data[0] >>= 8;
		data[0] |= (nTemp << 8);
	}

	if (AFP_Verify == 1)
	{
		for (i = 0; ( ( i < count ) && ( error == ERR_NONE ) );
				i++, offset += stride) {
			flash_unlock(offset);
			flash_write(offset, data[0]);
			error = flash_poll_toggle(offset);
			flash_read( offset, &cmp );
			if( cmp != (data[0]) ) {
				verify_err = 1;
				break;
			}

		}
		if( verify_err == 1 )
			return ERR_VERIFY_WRITE;
	}
	else
	{
		for (i = 0; (i < count) && (error == ERR_NONE);
				i++, offset += stride) {
			flash_unlock(offset);
			flash_write(offset, data[0]);
			error = flash_poll_toggle(offset);
		}
	}
	return error;
}


int flash_readchunk(unsigned long start, long count, long stride, FWORD *data)
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
	if( leftover > 0 ) {
		flash_read( offset, &data[i]);
	}

	return ERR_NONE;
}


int flash_writechunk(unsigned long start, long count,
		long stride, FWORD *data)
{
	long i = 0;
	unsigned long offset = start;
	int leftover = count % 2;		
	int error = ERR_NONE;
	FWORD cmp = 0;
	int verify_err = 0;

	if (count > AFP_Size) return ERR_BUFSIZE_EXCESS;

	stride <<= 1;

	// Write words
	for (i = 0; i < count / 2; i++) {
		flash_unlock(offset);
		flash_write(offset, data[i]);
		error = flash_poll_toggle(offset);
		if (error != ERR_NONE) return error;
		if (AFP_Verify) {
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
		flash_unlock(offset);
		flash_write(offset, data[i] | 0xff00);
		error = flash_poll_toggle(offset);
		if (AFP_Verify) {
			flash_read(offset, &cmp);
			if ((cmp & 0xff) != (data[i] & 0x00ff)) {
				verify_err = 1;
			}
		}
	}

	if (verify_err) error = ERR_VERIFY_WRITE;

	return error;
}

////////////////////////////////////////////////////////////////////////////
// Geometry auxiliaries

extern struct sector_desc *g_sectormap;
extern unsigned short g_mapsize;

int flash_get_sectaddr(unsigned long *start, unsigned long *end, int sector )
{
	int i;
	struct sector_desc *b = 0;
	int n = 0;

	for (i = 0; i < g_mapsize; i++) {
		b = &g_sectormap[i];
		n += b->n;
		if (sector < n)
			break;
	}

	if (!b) return ERR_INVALID_BLOCK;

	*start = b->base + sector * b->size;
	*end = *start + b->size - 1;

	return ERR_NONE;
}

int flash_get_sectno(unsigned long offset, int *secno)
{
	int s = 0;
	int i;
	struct sector_desc *b = 0;

	if (offset >= AFP_FlashSize) return ERR_INVALID_SECTOR;

	// Determine in which segment the address falls
	s = 0;
	for (i = 0; i < g_mapsize; i++) {
		b = &g_sectormap[i];
		if (offset < (b->base + b->n * b->size))
			break;
		s += b->n;
	}
	if (!b) return ERR_INVALID_BLOCK;

	// Determine offset index inside segment:
	s += (offset - b->base) / b->size;

	*secno = s;

	return ERR_NONE;
}

void flash_calc_size(void)
{
	int i;
	int sum;
	struct sector_desc *b;

	sum = 0;
	for (i = 0; i < g_mapsize; i++) {
		sum += g_sectormap[i].n;
	}

	AFP_NumSectors = sum;
	
	// Calculate flash size
	b = &g_sectormap[g_mapsize - 1];
	AFP_FlashSize = b->base + b->n * b->size;
}

int flash_erase_blk(int sector)
{
	unsigned long start, end;

	flash_get_sectaddr(&start, &end, sector);

	// send the erase block command to the flash
	
	return flash_sendcmd_blkerase(start);
}

int flash_erase()
{
	return flash_sendcmd_fullerase();
}


////////////////////////////////////////////////////////////////////////////
// MAIN

int main()
{
	ebiu_init();
	flash_get_codes();
	AFP_Error = flash_init(AFP_DevCode);
	if (AFP_Error == 0)
		flash_calc_size();

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
				AFP_Error = ERR_UNKNOWN_COMMAND;
				break;
		}
		AFP_Command = NO_COMMAND;
	}
	return 0;
}


