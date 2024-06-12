/** \file bfloader.h 
 *
 * bfloader backend API
 *
 * (c) 12/2005 Martin Strubel <hackfin@section5.ch>
 *
 */


// structure for flash sector information
typedef struct sector_location_s {
	unsigned long start;
	unsigned long end;
} SectorLocation;

// Flash Programmer commands
typedef enum
{
	NO_COMMAND,		// 0
	GET_CODES,		// 1
	RESET,			// 2
	WRITE,			// 3
	FILL,			// 4
	ERASE_ALL,		// 5
	ERASE_SECT,		// 6
	READ,			// 7
	GET_SECTNUM,	// 8
	GET_SECSTARTEND	// 9
} ProgCmd;

// common helpers

int flash_sendcmd_blkerase(unsigned long sector_addr);
int flash_sendcmd_fullerase();

// pretty common routines (common.c)
int flash_init();
int flash_get_codes();
int flash_unlock(unsigned long offset);
int flash_writechunk(unsigned long start, long count, long stride, short *data);
int flash_fill(unsigned long start, long count, long stride, short *data);
int flash_readchunk(unsigned long start, long count, long stride, short *data);

// Platform specific routines:
int flash_reset();
int flash_read(unsigned long offset, short *value);
int flash_write(unsigned long offset, int value);
int flash_poll_toggle(unsigned long offset);
int flash_get_sectno(unsigned long offset, int *secno);
int flash_get_sectaddr(unsigned long *start, unsigned long *end, int secno);

int flash_erase();
int flash_erase_blk(int block);

// LED monitor
//

void set_led(int state);
