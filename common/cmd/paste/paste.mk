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

#ident	"@(#)paste:paste.mk	1.2.5.1"
#ident "$Header: paste.mk 1.2 91/04/16 $"

include $(CMDRULES)

#	Makefile for paste

OWN = bin
GRP = bin

LDLIBS = -lw

all: paste

paste: paste.o
	$(CC) -o paste paste.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

paste.o: paste.c \
	$(INC)/stdio.h \
	$(INC)/sys/euc.h \
	$(INC)/getwidth.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) paste

clean:
	rm -f paste.o

clobber: clean
	rm -f paste

lintit:
	$(LINT) $(LINTFLAGS) paste.c

