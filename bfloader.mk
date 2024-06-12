# Common makefile for bfloader stuff
#
#
OBJS = main.o

ifndef NO_COMMON
	OBJS += common.o
endif

NAME = bfloader

CC =      bfin-elf-gcc
LD =      bfin-elf-ld
LINKERSCRIPT = bftiny.x

CFLAGS = -I../../include -I../include -I..
CFLAGS += -Wall
CFLAGS += -D__ADSPLPBLACKFIN__ -D__ADSPBLACKFIN__
CFLAGS += -DBOARD_$(BOARD)

ifdef DEBUG
	CFLAGS += -g
endif

LDFLAGS =  -Tbftiny.x
LDFLAGS += -B.


all : $(OBJS) crt0.o
	$(CC) $(LDFLAGS) -o $(NAME).dxe $(OBJS)

crt0.o : ../crt0.asm
	$(CC) -c -x assembler-with-cpp -o $@ $< $(CFLAGS) 

common.o: ../common/common.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.dat


