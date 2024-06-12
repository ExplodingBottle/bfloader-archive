/********************************************************************
 * All config settings for blackfin startup
 *
 */

/* PLL and clock setup values:
 */


// Customize:
// Formulas for clock speeds:
//

// make sure, VCO stays below 600 Mhz
// VCO = VCO_MULTIPLIER * MASTER_CLOCK / MCLK_DIVIDER
//
// where MCLK_DIVIDER = 1 when DF bit = 0,  (default)
//                      2               1
//
// CCLK = VCO / CCLK_DIVIDER
//
// SCLK = VCO / SCLK_DIVIDER
//

#ifdef BOARD_EZKIT_BF533

#define MASTER_CLOCK   27000000
#define SCLK_DIVIDER   4
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


