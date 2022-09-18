#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/devices/erase/Makefile	1.2"
#ident "$Header: Makefile 1.1 91/08/29 $"

include $(CMDRULES)

INSDIR = $(USRSADM)/sysadm/bin

O_CFILES = spclsize
O_SHFILES=\
	floperase
O_DFILES=\
	Help \
	Menu.erase Form.erase \
	Text.erase2 Text.erase Text.eraserr

.MUTEX: all xinstall

all: $(O_SHFILES) $(O_CFILES)

$(O_CFILES): $(O_CFILES).o
	$(CC) $(CFLAGS) $(DEFLIST) -o $(O_CFILES) $(O_CFILES).o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(O_CFILES).o:	$(O_CFILES).c \
		$(INC)/fcntl.h $(INC)/stdio.h \
		$(INC)/signal.h $(INC)/values.h 

clean:
	rm -f *.o

clobber: clean
	rm -f $(O_CFILES) $(O_SHFILES)

lintit:
	$(LINT) $(DEFLIST) $(O_CFILES) 

strip size: all

install: all xinstall

PROTO=Prototype
OBJECTS=$(O_CFILES) $(O_SHFILES) $(O_DFILES)
include ../../Install
