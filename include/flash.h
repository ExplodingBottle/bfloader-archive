
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

// Specific Intel commands

#define SR_READY        0x80
#define SR_SUSP_ERASE   0x40
#define SR_ERR_ERASE    0x20
#define SR_ERR_PROG     0x10
#define SR_ERR_VOLTAGE  0x08
#define SR_SUSP_PROG    0x04
#define SR_ERR_LOCKED   0x02

#define ICMD_PROG_BYTE  0x10
#define ICMD_BLKERASE   0x20
#define ICMD_PROG_WORD  0x40
#define ICMD_CLRSTATUS  0x50
#define ICMD_LOCK       0x60
#define ICMD_READID     0x90
#define ICMD_ERASECONF  0xd0
#define ICMD_READARRAY  0xff


#define ADDR_DEV        0x000
#define ADDR_ID         0x002
