#
# Prototype routines to drive VDSP loader backend using GDB
# <hackfin@section5.ch>
#

# Example usage:

# To read 512 bytes from Flash from offset 0, use:
# 
# (bfloader) readfile 0 0x200
#
# Note that the data is appended to the file 'out.dat'. You have to
# remove this file, if existing

# To erase the flash:
#
# (bfloader) erase_all

# To write 0x8000 bytes to the Flash at offset 0x10000:
#
# The data must be in the file '/tmp/flash.dat'.
#
# (bfloader) writefile 0x10000 0x8000

# Verify flash write with a following read and compare, if == 1
set $VERIFY = 1

set $CMD_GET_CODES          = 1
set $CMD_RESET              = 2
set $CMD_WRITE              = 3
set $CMD_FILL               = 4
set $CMD_ERASE_ALL          = 5
set $CMD_ERASE_SECT         = 6
set $CMD_READ               = 7
set $CMD_GET_SECTN          = 8
set $CMD_GET_SEC_START_END  = 9

# executes command on target
define exec_command
	set AFP_Command = $arg0
	c
end

set height 0

define dump_error
	if $arg0 == 1
		echo Poll timeout
	end
	if $arg0 == 2
		echo Verify failed
	end
	if $arg0 == 3
		echo Invalid sector
	end
	if $arg0 == 4
		echo Invalid block
	end
	if $arg0 == 5
		echo Unknown command
	end
	if $arg0 == 6
		echo Error when processing command
	end
	if $arg0 == 7
		echo Read error
	end
	if $arg0 == 8
		echo Fatal: Driver not at break
	end
	if $arg0 == 9
		echo Buffer is invalid
	end
	if $arg0 == 10
		echo Block size too big, check AFP_Size
	end
end

define readfile
	set $offset = $arg0
	set $size = $arg1

	set $error = 0

	while ($size > 0 && $error == 0)
		if $size > AFP_Size
			set $count = AFP_Size
		else
			set $count = $size
		end
		set AFP_Offset = $offset
		set AFP_Count = $count
		printf "Reading..."
		exec_command $CMD_READ
		printf "done.\n"
		set $error = AFP_Error
		set $offset = $offset + $count
		set $size = $size - $count
		set $end = (char *) AFP_Buffer + $count

		append memory out.dat AFP_Buffer $end
	end

	if $error
		printf "Error %d occured\n", $error
	end
end


define writefile
	set $offset = $arg0
	set $size = $arg1

	set $error = 0
	set $pos = 0

	set $addr = (char *) AFP_Buffer

	while ($size > 0 && $error == 0)
		if $size > AFP_Size
			set $count = AFP_Size
		else
			set $count = $size
		end
		set AFP_Offset = $offset
		set AFP_Count = $count

		set $end = $pos + $count
		restore /tmp/flash.dat binary $addr $pos $end
		printf "Flashing block @ 0x%08lx\n", $pos
		exec_command $CMD_WRITE

		set $error = AFP_Error
		set $offset = $offset + $count
		set $pos = $pos + $count
		set $size = $size - $count
		# This is a very stupid offset fixup we have to do
		# for the 'restore' command
		set $addr = $addr - $count
	end

	if $error
		printf "--------------------------\n"
		printf "-- Error %02d             --\n", $error
		printf "   "
		dump_error $error
		echo \n
		printf "--------------------------\n"
	end
end

define erase_all
	echo Erasing entire flash - this can take some time. Do not interrupt.
	exec_command $CMD_ERASE_ALL
end

define erase_blocks
	set $sect = $arg0
	set $end = $sect + $arg1
	while ($sect < $end)
		printf "erase block %d\n", $sect
		set AFP_Sector = $sect
		exec_command $CMD_ERASE_SECT
		set $sect = $sect + 1
	end
end

define reset_flash
	printf "Resetting flash...\n"
	exec_command $CMD_RESET
end

define exit
	monitor reset
	detach
	q
end


