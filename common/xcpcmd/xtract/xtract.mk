#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcpxtract:xtract.mk	1.2.2.2"
#ident  "$Header: xtract.mk 1.2 91/07/11 $"

include $(CMDRULES)

#	Makefile for xtract

OWN = root
GRP = sys

LDLIBS = -lgen

all: xtract

xtract: xtract.o
	$(CC) -o xtract xtract.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

xtract.o: xtract.c \
	$(INC)/stdio.h \
	$(INC)/signal.h $(INC)/sys/signal.h

install: all
	 $(INS) -f $(USRBIN) -u $(OWN) -m 0755 -g $(GRP) xtract

clean:
	rm -f xtract.o

clobber: clean
	rm -f xtract

lintit:
	$(LINT) $(LINTFLAGS) xtract.c

