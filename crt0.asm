//
// crt0.asm
//
// This is the startup code to get a supervisor program going.
//
// 2004, 2005 Martin Strubel <hackfin@section5.ch>
//
// $Id: crt0.asm,v 1.1 2006/01/01 17:58:11 strubi Exp $
//
//
//


// .extern _report_exception;

.section .init;

#include <defBF533.h>

// some exception handling macros
#include "exception.h"
 
// core clock dividers -- DO NOT CHANGE!
#define CCLK_1 0x00
#define CCLK_2 0x10
#define CCLK_4 0x20
#define CCLK_8 0x30

#include "config.h"

// little macro trick to resolve macros before concatenating:
#define _GET_CCLK(x) CCLK_##x
#define GET_CCLK(x) _GET_CCLK(x)
 


// Short bootstrap

.global start

start:

	sp.h = 0xFFB0;		//Set up supervisor stack in scratch pad
	sp.l = 0x0400;
	fp = sp;

// TEST stuff XXX remove 

	[--sp] = r1;
	r1 = [sp++];


////////////////////////////////////////////////////////////////////////////
// PLL and clock setups
//
//



setupPLL:
	// we have to enter the idle state after changes applied to the
	// VCO clock, because the PLL needs to lock in on the new clocks.


	p0.l = lo(PLL_CTL);
	p0.h = hi(PLL_CTL);
	r1 = w[p0](z);
	r2 = r1;  
	r0 = 0(z);
		
	r0.l = ~(0x3f << 9);
	r1 = r1 & r0;
	r0.l = ((VCO_MULTIPLIER & 0x3f) << 9);
	r1 = r1 | r0;

 	p1.l = LO(SIC_IWR);  // enable PLL Wakeup Interrupt
	p1.h = HI(SIC_IWR);
	r0 = [p1];			
	bitset(r0,0);	  
	[p1] = r0;
	
 	w[p0] = r1;          // Apply PLL_CTL changes.
	ssync;
 	
	cli r0;
 	idle;	// wait for Loop_count expired wake up
	sti r0;


	// now, set clock dividers:
	p0.l = LO(PLL_DIV);
	p0.h = HI(PLL_DIV);


	// SCLK = VCOCLK / SCLK_DIVIDER
	r0.l = (GET_CCLK(CCLK_DIVIDER) | (SCLK_DIVIDER & 0x000f));


	w[p0] = r0; // set Core and system clock dividers


	// not needed in reset routine: sti r1;

////////////////////////////////////////////////////////////////////////////
// install default interrupt handlers

	p0.l = lo(EVT2);
	p0.h = hi(EVT2);

	r0.l = _NHANDLER;
	r0.h = _NHANDLER;  	// NMI Handler (Int2)
    [p0++] = r0;

    r0.l = EXC_HANDLER;
  	r0.h = EXC_HANDLER;  	// Exception Handler (Int3)
    [p0++] = r0;
	
	[p0++] = r0; 		// IVT4 isn't used

    r0.l = _HWHANDLER;
	r0.h = _HWHANDLER; 	// HW Error Handler (Int5)
    [p0++] = r0;
	
    r0.l = _THANDLER;
	r0.h = _THANDLER;  	// Timer Handler (Int6)
	[p0++] = r0;
	
    r0.l = _RTCHANDLER;
	r0.h = _RTCHANDLER; // IVG7 Handler
	[p0++] = r0;
	
    r0.l = _I8HANDLER;
	r0.h = _I8HANDLER; 	// IVG8 Handler
  	[p0++] = r0;
  	
  	r0.l = _I9HANDLER;
	r0.h = _I9HANDLER; 	// IVG9 Handler
 	[p0++] = r0;
 	
    r0.l = _I10HANDLER;
	r0.h = _I10HANDLER;	// IVG10 Handler
 	[p0++] = r0;
 	
    r0.l = _I11HANDLER;
	r0.h = _I11HANDLER;	// IVG11 Handler
  	[p0++] = r0;
  	
    r0.l = _I12HANDLER;
	r0.h = _I12HANDLER;	// IVG12 Handler
  	[p0++] = r0;
  	
    r0.l = _I13HANDLER;
	r0.h = _I13HANDLER;	// IVG13 Handler
    [p0++] = r0;

    r0.l = _I14HANDLER;
	r0.h = _I14HANDLER;	// IVG14 Handler
  	[p0++] = r0;

    r0.l = _I15HANDLER;
	r0.h = _I15HANDLER;	// IVG15 Handler
	[p0++] = r0;

#ifdef NOT_WORKING

////////////////////////////////////////////////////////////////////////////
// Setup CPLBs
// We need this for referencing external memory code,
// otherwise, VisualDSP++ will barf.
//

	// by default, cache and ICPLBs are disabled.
	// so we can start setting up the CPLBs now:

	// setup up CPLB for external memory:

	p0.l = LO(ICPLB_DATA0);
	p0.h = HI(ICPLB_DATA0);

	// valid, locked, user readable and cacheable (enable L1 Instr. Cache!)
	r0 = (CPLB_VALID | CPLB_LOCK | CPLB_USER_RD | CPLB_L1_CHBL ) (z);
	// Grab a page in SDRAM :-)
	r1.l = LO(PAGE_SIZE_1MB);
	r1.h = HI(PAGE_SIZE_1MB);

	r0 = r0 | r1;  

	[p0] = r0;  // write to IPCLB_DATA0

	p0.l = LO(ICPLB_ADDR0);
	p0.h = HI(ICPLB_ADDR0);
	
	r0.l = LO(0x100000);
	r0.h = HI(0x100000);

	[p0] = r0;  


	// now, configure:

	r0 = (ENICPLB | IMC ) (z);  // enable CPLB and caching.

	p0.l = LO(IMEM_CONTROL);
	p0.h = HI(IMEM_CONTROL);

	[p0] = r0;  // write to IMEM_CONTROL
	csync;


// end CPLB setup
////////////////////////////////////////////////////////////////////////////
#endif

	// we want to run our program in supervisor mode,
	// therefore we need a few tricks:

	//  Enable Interrupt 15 
	p0.l = lo(EVT15);
	p0.h = hi(EVT15);
	r0.l = call_main;  // install isr 15 as caller to main
	r0.h = call_main;
	[p0] = r0;

	r0 = 0x8000(z);    // enable irq 15 only
	sti r0;            // set mask
	raise 15;          // raise sw interrupt
	
	p0.l = wait;
	p0.h = wait;

	reti = p0;
	rti;               // return from reset

wait:
	jump wait;         // wait until irq 15 is being serviced.



call_main:
	[--sp] = reti;  // pushing RETI allows interrupts to occur inside all main routines


	p0.l = _main;
	p0.h = _main;

	r0.l = end;
	r0.h = end;

	rets = r0;      // return address for main()'s RTS

	jump (p0);

end:
	idle;
	jump end;


idle_loop:
	idle;
	ssync;
	jump idle_loop;


start.end:

////////////////////////////////////////////////////////////////////////////
// SETUP ROUTINES
//




////////////////////////////////////////////////////////////////////////////
// Default handlers:
//


// If we get caught in one of these handlers for some reason, 
// display the IRQ vector on the EZKIT LEDs and enter
// endless loop.

display_fail:
	bitset(r0, 5);    // mark error
	jump stall;


_HWHANDLER:           // HW Error Handler 5
rti;

_NHANDLER:
stall:
	jump stall;

EXC_HANDLER:          // exception handler
	r0 = seqstat;
	r1 = retx;
	// call _report_exception
	cc = r0 == 0;
	if !cc jump cont_program;
	jump idle_loop;
cont_program:
	rtx;

_THANDLER:            // Timer Handler 6	
	r0.l = 6;
	jump display_fail;

_RTCHANDLER:          // IVG 7 Handler  
	r0.l = 7;
	jump display_fail;

_I8HANDLER:           // IVG 8 Handler
	r0.l = 8;
	jump display_fail;

_I9HANDLER:           // IVG 9 Handler
	r0.l = 9;
	jump display_fail;

_I10HANDLER:          // IVG 10 Handler
	r0.l = 10;
	jump display_fail;

_I11HANDLER:          // IVG 11 Handler
	r0.l = 11;
	jump display_fail;

_I12HANDLER:          // IVG 12 Handler
	r0.l = 12;
	jump display_fail;

_I13HANDLER:		  // IVG 13 Handler
	r0.l = 13;
	jump display_fail;
 
_I14HANDLER:		  // IVG 14 Handler
	r0.l = 14;
	jump display_fail;

_I15HANDLER:		  // IVG 15 Handler
	r0.l = 15;
	jump display_fail;
	
	
////////////////////////////////////////////////////////////////////////////
// we need _atexit if we don't use a libc..

#ifdef USE_NO_LIBC
.global _atexit;
_atexit:
	rts;

#endif
