VERSION = 0.1

dist:
	tar cfz ../bfloader-$(VERSION).tgz -C .. bfloader \
	--exclude=*.o --exclude=.*.swp --exclude=CVS

