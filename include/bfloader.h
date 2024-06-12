/** \file bfloader.h 
 *
 * bfloader backend API
 *
 * (c) 12/2005 Martin Strubel <hackfin@section5.ch>
 *
 */

#ifdef FLASH_32BIT
#define FWORD long
#else
#define FWORD short
#endif


// structure for flash sector information
typedef struct sector_location_s {
	unsigned long start;
	unsigned long end;
} SectorLocation;

struct sector_desc
{
	unsigned long base;   // sector base address
	short         n;      // number of sectors of same size
	unsigned long size;   // sector size in bytes
};

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
void ebiu_init(void);
int flash_init(int code);
int flash_get_codes();
int flash_unlock(unsigned long offset);

// Platform specific routines:
int flash_reset();

int flash_read(unsigned long offset, FWORD *value);
int flash_write(unsigned long offset, FWORD value);
int flash_writechunk(unsigned long start, long count, long stride, FWORD *data);
int flash_fill(unsigned long start, long count, long stride, FWORD *data);
int flash_readchunk(unsigned long start, long count, long stride, FWORD *data);

int flash_poll_toggle(unsigned long offset);
int flash_get_sectno(unsigned long offset, int *secno);
int flash_get_sectaddr(unsigned long *start, unsigned long *end, int secno);

int flash_erase();
int flash_erase_blk(int block);

void flash_calc_size(void);

// LED monitor
//

void set_led(int state);
