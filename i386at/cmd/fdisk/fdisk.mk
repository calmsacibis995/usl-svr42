#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fdisk:i386at/cmd/fdisk/fdisk.mk	1.9"

include	$(CMDRULES)

MAINS	= fdisk 

ALL:		$(MAINS)

fdisk.o:	fdisk.c

fdisk:	fdisk.o
	$(CC) $(LDFLAGS) -o ./fdisk fdisk.o -lcurses -lgen


fdisk.c:            \
		 $(INC)/sys/types.h \
		 $(INC)/sys/vtoc.h \
		 $(INC)/sys/termios.h \
		 $(INC)/sys/fdisk.h \
		 $(INC)/curses.h \
		 $(INC)/term.h \
		 $(INC)/fcntl.h \
		 $(INC)/libgen.h

clean:
	rm -f fdisk.o 

clobber:        clean
	rm -f fdisk 

all : ALL

install: ALL
	$(INS) -f $(USRSBIN) fdisk
