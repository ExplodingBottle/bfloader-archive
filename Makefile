# Makefile for bfloader flash tools
#
# (c) section5 <hackfin@section5.ch>
# 

VERSION = 1.01beta

SUBDIRS = ezkit-bf561 stamp zbrain tinyboards

DIST = bfloader.gdb bfloader.mk common/common.c config.h crt0.asm  \
	include Makefile README LICENSE doc/bfloader.html doc/images \
	loadelf.c main.c error.c ../include \
	ChangeLog $(SUBDIRS)

DISTFILES = $(DIST:%=bfloader/%)

CFLAGS = -Wall -g -Iinclude -I../bfemu -DVERSION_STRING=\"$(VERSION)\"

PLATFORM = $(shell uname)

ifeq (CYGWIN_NT-5.1,$(PLATFORM))
	CFLAGS += -I/usr/local/include
	LDFLAGS = -L../bfemu/lib -L/usr/local/lib -lftd2xx 
else
	LDFLAGS = -lftdi  -L../bfemu
endif

LDFLAGS += -lbfemu -lelf

OBJS = loadelf.o main.o error.o

flashload: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

all: flashload subdirs_all

subdirs_%:
	for i in ezkit-bf561 $(SUBDIRS); do \
		$(MAKE) -C $$i $*;\
	done

clean: subdirs_clean
	rm -f flashload $(OBJS)

distclean: subdirs_clean

dist: distclean
	tar cfz ../bfloader-$(VERSION).tgz -C .. $(DISTFILES) \
	--exclude=CVS --exclude=*.o --exclude=*.txt --exclude=zbrain/images

DISTDIR = flashloader

zipdist:
	[ -e $(DISTDIR) ] || mkdir $(DISTDIR)
	cp /bin/cygwin1.dll $(DISTDIR)
	cp /bin/bfemu.dll $(DISTDIR)
	strip $(DISTDIR)/bfemu.dll
	strip flashload.exe
	cp flashload.exe $(DISTDIR)
	cp flashinfo.bat $(DISTDIR)
	cp svdo/bfloader.dxe $(DISTDIR)
	zip -r $(DISTDIR).zip $(DISTDIR)
	rm -fr $(DISTDIR)

