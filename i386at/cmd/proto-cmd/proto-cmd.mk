#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto-cmd:proto-cmd.mk	1.3.4.2"

#       Makefile for Packaging and Installation Commands

include	$(CMDRULES)


LOCALDEF = -DAT386

ALL	= \
	machine_type \
	machine_type.dy \
	links \
	setmods \
	setmods.elf \
	mkflist \
	contents \
	x286 \
	bootcntl \
	check_uart

NOT_FOUND= \
	bdiff.nf  \
	adv.nf  \
	300.nf  \
	crypt.nf  \
	nlsadmin.nf  \
	fsba.nf

DIRS	= \
	$(VAR)/sadm/pkg/dfm \
	$(VAR)/tmp

all: $(ALL) pkginfo default

install: all $(DIRS)
	$(INS) -f $(USRBIN) machine_type
	$(INS) -f $(USRBIN) machine_type.dy
	$(INS) -f $(USRBIN) bootcntl
	$(INS) -f $(ETC) links
	$(INS) -f $(ETC) setmods
	$(INS) -f $(ETC) setmods.elf
	$(INS) -f $(ETC) mkflist
	$(INS) -f $(ETC) contents
	$(INS) -f $(ETC) check_uart
	$(INS) -f $(USRBIN) x286
	cp pkginfo $(VAR)/sadm/pkg/dfm/pkginfo
	cp default $(VAR)/default

$(DIRS):
	-mkdir -p $@

fixswap: fixswap.c
	cc -s $(CFLAGS) -o $@ $?
	echo fixswap | sh ../../proto/i386/prep NO_UNIX

machine_type: machine_type.c
	$(CC) -O -s -o $@ -I$(INC) $? $(ROOTLIBS)

machine_type.dy: machine_type.c
	$(CC) -O -s -o $@ -I$(INC) $? 

links: links.c
	$(CC) -s $(CFLAGS) -o $@ $? $(SHLIBS)

x286: x286.c
	$(CC) -s $(CFLAGS) -o $@ $? $(SHLIBS)

setmods contents mkflist:
	cc -s $(CFLAGS) -o $@ $@.c

setmods.elf: setmods.c
	$(CC) -s $(CFLAGS) -o $@ $? $(SHLIBS)

bootcntl:	bootcntl.c
	$(CC) -s $(CFLAGS) -I$(INC) -o $@ $@.c

check_uart:	check_uart.c
	$(CC) -s $(CFLAGS) -I$(INC) -o $@ $? $(ROOTLIBS)

clean:
	-$(RM) -f *.o

clobber: clean
	-$(RM) -f $(ALL)
