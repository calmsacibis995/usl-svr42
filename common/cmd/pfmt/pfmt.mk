#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pfmt:pfmt.mk	1.1.3.2"
#ident "$Header: pfmt.mk 1.3 91/05/23 $"

include $(CMDRULES)

#	Makefile for pfmt

LDLIBS = -lgen
MAINS = lfmt pfmt

OBJECTS = lfmt.o pfmt.o
SOURCES = $(OBJECTS:.o=.c)

all: $(MAINS)

lfmt: lfmt.o
	$(CC) -o lfmt lfmt.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

pfmt: pfmt.o 
	$(CC) -o pfmt pfmt.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

pfmt.o: pfmt.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h

lfmt.o: lfmt.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h
	$(CC) $(CFLAGS) $(DEFLIST) -DLFMT -c lfmt.c
	rm -f lfmt.c

lfmt.c: pfmt.c
	cp pfmt.c lfmt.c

install: all
	$(INS) -f $(USRBIN) -m 0555 -u bin -g bin pfmt
	$(INS) -f $(USRBIN) -m 0555 -u bin -g bin lfmt

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) pfmt.c

