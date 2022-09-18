#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/instcmd/instcmd.mk	1.9.1.5"
#ident	"$Header: $

include	$(CMDRULES)

MAINS	= check_devs stepper instcmd edsym
OBJECTS	= check_devs.o stepper.o instcmd.o edsym.o
SOURCES	= check_devs.c stepper.c instcmd.c edsym.c

all: $(MAINS)

install: all
	$(INS) -f ../ifiles -m 555 -u root -g other check_devs
	$(INS) -f ../ifiles -m 555 -u root -g other stepper
	$(INS) -f ../ifiles -m 555 -u root -g other instcmd

check_devs:	check_devs.o
	$(CC) -o check_devs check_devs.o $(LDFLAGS)

stepper:	stepper.o
	$(CC) -o stepper stepper.o $(ROOTLIBS)

edsym:	edsym.o
	$(CC) -o edsym edsym.o $(LDFLAGS) 

instcmd:	instcmd.o
	$(CC) -o instcmd instcmd.o $(LDFLAGS) 

check_devs.o:	$(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/sys/cram.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fd.h \
	$(INC)/fcntl.h \
	$(INC)/string.h 


stepper.o: stepper.c \
	$(INC)/sys/types.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/stermio.h \
	$(INC)/sys/termiox.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/at_ansi.h \
	$(INC)/sys/kd.h \
	$(INC)/signal.h \
	$(INC)/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)
