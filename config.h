/********************************************************************
 * All config settings for Blackfin startup code
 *
 */


#ifdef BOARD_EZKIT_BF533

#define MASTER_CLOCK   27000000
#define SCLK_DIVIDER   4
#define VCO_MULTIPLIER 16
#define CCLK_DIVIDER   1

#endif

#ifdef BOARD_EZKIT_BF561

#define MASTER_CLOCK   30000000
#define SCLK_DIVIDER   5
#define VCO_MULTIPLIER 16
#define CCLK_DIVIDER   1

#endif

#ifdef BOARD_STAMP_BF533

#define MASTER_CLOCK   11000000
#define SCLK_DIVIDER   5
#define VCO_MULTIPLIER 32
#define CCLK_DIVIDER   1

#endif

#ifdef BOARD_STAMP_BF537

#define MASTER_CLOCK   25000000
#define SCLK_DIVIDER   4
#define VCO_MULTIPLIER 16
#define CCLK_DIVIDER   1

#endif

#ifdef BOARD_DSPSTAMP

#define MASTER_CLOCK   25000000
#define SCLK_DIVIDER   5
#define VCO_MULTIPLIER 16
#define CCLK_DIVIDER   1

#endif

#ifdef BOARD_ZBRAIN_BF533

#define MASTER_CLOCK   24000000
#define SCLK_DIVIDER   5
#define VCO_MULTIPLIER 16
#define CCLK_DIVIDER   1

#endif

#ifdef BOARD_CM_BF533

#define MASTER_CLOCK   25000000
#define SCLK_DIVIDER   5
#define VCO_MULTIPLIER 16
#define CCLK_DIVIDER   1

#endif

#ifdef BOARD_SVDO_BF539

#define MASTER_CLOCK   16000000
#define SCLK_DIVIDER   5
#define VCO_MULTIPLIER 20
#define CCLK_DIVIDER   1

#endif


