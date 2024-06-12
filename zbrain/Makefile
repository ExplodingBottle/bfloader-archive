
BOARD = ZBRAIN_BF533

include ../bfloader.mk

IMAGES = $(HOME)/uClinux-dist/images/jffs2.img 
IMAGES += $(HOME)/uClinux-dist/images/vmImage

mrproper: clean
	rm -fr images

dist: clean
	[ -e images ] || mkdir images
	cp $(IMAGES) images
	tar cfz ../zbrain-loader.tgz -C .. zbrain
