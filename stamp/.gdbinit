#
# BFloader script for STAMP boards (533, 537)
# <hackfin@section5.ch>
#

source ../bfloader.gdb

define program_uboot
	erase_blocks 0 19
	source loaduboot.gdb
end

define program_linux
	erase_blocks 20 48
	source loadlinux.gdb
end

define program_all
	erase_all
	source loaduboot.gdb
	source loadlinux.gdb
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

echo Resetting Flash\n
reset_flash

echo ////////////////////////////////////////////////////////////\n
echo // BFLOADER script v0.1 [STAMP customized]\n
echo ////////////////////////////////////////////////////////////\n\n
echo Available commands:\n\n
echo program_uboot  - Erase up to 0x20100000 and flash with u-boot\n
echo program_linux  - Erase from  0x20100000 and flash with linux+rootfs\n
echo program_all    - Erase entire chip and flash with u-boot, linux+rootfs\n
echo exit           - Reset target and leave bfloader\n
echo \n
if AFP_Verify == 1
	echo Verify is on (AFP_Verify = 1)
else
	echo Verify is off (AFP_Verify = 0)
end
echo \n\n
