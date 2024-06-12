#
# BFloader script for ZBrain images (vmImage + jffs2.img)
#
# <hackfin@section5.ch>
#

source ../bfloader.gdb

define program_all
	erase_all
	source loadlinux.gdb
	source loadrootfs.gdb
end

define program_linux
	erase_blocks 0 32
	source loadlinux.gdb
end

define exit
	monitor reset
	detach
	q
end

# MAIN program

set prompt (bfloader)\ 
target remote :2000

monitor reset

load bfloader.dxe

set AFP_Stride = 1
set AFP_Sector = 0
set AFP_Verify = $VERIFY

#display AFP_Error

b *&AFP_BreakReady

c

# Async Bank configuration registers
set $EBIU_AMGCTL  = (unsigned short *) 0xffc00a00 
set $EBIU_AMBCTL0 = (unsigned long *)  0xffc00a04 
set $EBIU_AMBCTL1 = (unsigned long *)  0xffc00a08 

# These are specific timings for your board
set *$EBIU_AMBCTL0 = 0xf3c0ffc0
set *$EBIU_AMBCTL1 = 0xffc2ffc0
# enable all banks
set *$EBIU_AMGCTL  = 0x0f

echo Resetting Flash\n
reset_flash

echo ////////////////////////////////////////////////////////////\n
echo // BFLOADER script v0.1 [ZBrain customized]\n
echo ////////////////////////////////////////////////////////////\n\n
echo Available commands:\n\n
echo program_all     - Erase entire Flash and program with image(s)\n
echo program_linux   - Overwrite first partition of flash with linux only\n
echo exit            - Reset target and leave bfloader\n
echo \n
if AFP_Verify == 1
	echo Verify is on (AFP_Verify = 1)
else
	echo Verify is off (AFP_Verify = 0)
end
echo \n\n
