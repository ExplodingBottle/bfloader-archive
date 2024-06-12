#!/bin/sh

# Generates a flash loader GDB script for the images below

# (c) 11/2005, Martin Strubel <hackfin@section5.ch>

linux=./images/vmImage
rootfs=./images/jffs2.img

offset_linux=0x00000000
offset_rootfs=0x00100000

linux_loader=loadlinux.gdb
rootfs_loader=loadrootfs.gdb

echo "# GDB flash loader script" > $linux_loader
echo "# Automatically generated by 'flashload.sh'" >> $linux_loader
echo "#    (modifications will be overwritten)" >> $linux_loader

# Determine size
echo "set \$size =" `du -b $linux | sed 's/^\([0-9]*\).*/\1/'` >> $linux_loader
echo "shell cp $linux /tmp/flash.dat" >> $linux_loader
echo "writefile $offset_linux \$size" >> $linux_loader


echo "# GDB flash loader script" > $rootfs_loader
echo "# Automatically generated by 'flashload.sh'" >> $rootfs_loader
echo "#    (modifications will be overwritten)" >> $rootfs_loader

# Determine size
echo "set \$size =" `du -b $rootfs | sed 's/^\([0-9]*\).*/\1/'` >> $rootfs_loader
echo "shell cp $rootfs /tmp/flash.dat" >> $rootfs_loader
echo "writefile $offset_rootfs \$size" >> $rootfs_loader

bfin-jtag-gdb bfloader.dxe