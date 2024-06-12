/* Exception/interrupt handling macros
 *
 * 08/2004 Martin Strubel <hackfin@section5.ch>
 *
 * $Id: exception.h,v 1.1 2006/01/01 17:58:11 strubi Exp $
 *
 */

.macro	save_all
	[--sp] = ( r7:1, p5:0 );
	[--sp] = syscfg;	/* store SYSCFG */
	[--sp] = fp;
	[--sp] = usp;
	[--sp] = a0.x;
	[--sp] = a1.x;
	[--sp] = a0.w;
	[--sp] = a1.w;
	[--sp] = rets;
	[--sp] = astat;

.endm

.macro restore_all
	astat = [sp++];
	rets = [sp++];
	a1.w = [sp++];
	a0.w = [sp++];
	a1.x = [sp++];
	a0.x = [sp++];
	usp = [sp++];
	fp = [sp++];
	syscfg = [sp++];
	( r7:1, p5:0 ) = [sp++];

.endm

