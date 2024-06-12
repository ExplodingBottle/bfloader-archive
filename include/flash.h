
// Flash write sequences for a 16 bit flash

#define WRITESEQ1		0x0AAA
#define WRITESEQ2		0x0554
#define WRITESEQ3		0x0AAA
#define WRITESEQ4		0x0AAA
#define WRITESEQ5		0x0554
#define WRITESEQ6		0x0AAA
#define WRITEDATA1		0xaa
#define WRITEDATA2		0x55
#define WRITEDATA3		0x80
#define WRITEDATA4		0xaa
#define WRITEDATA5		0x55
#define WRITEDATA6		0x10


// Status register bits:
//

#define SR_DATAPOLL   0x80
#define SR_TOGGLE     0x40
#define SR_ERROR      0x20
#define SR_TIMER      0x08
#define SR_ALT_TOGGLE 0x04

#define CMD_RESET     0xf0
#define CMD_BLKERASE  0x30

#define CMD_UNLOCK1	    0xaa
#define CMD_UNLOCK2     0x55
#define CMD_UNLOCK3     0xa0

#define CMD_GETCODE1    0xaa
#define CMD_GETCODE2    0x55
#define CMD_GETCODE3    0x90

#define ADDR_DEV        0x400
#define ADDR_ID         0x402
