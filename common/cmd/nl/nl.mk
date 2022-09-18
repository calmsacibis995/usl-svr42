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

#ident	"@(#)nl:nl.mk	1.4.4.1"
#ident "$Header: nl.mk 1.2 91/04/16 $"

include $(CMDRULES)

#	Makefile for nl

OWN = bin
GRP = bin

LDLIBS = -lgen

all: nl

nl: nl.o
	$(CC) -o nl nl.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

nl.o: nl.c \
	$(INC)/stdio.h \
	$(INC)/regexpr.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) nl

clean:
	rm -f nl.o

clobber: clean
	rm -f nl


lintit:
	$(LINT) $(LINTFLAGS) nl.c

