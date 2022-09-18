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

#ident	"@(#)tabs:tabs.mk	1.11.1.3"
#ident "$Header: tabs.mk 1.3 91/06/06 $"

include $(CMDRULES)

#	Makefile for tabs

OWN = bin
GRP = bin

LDLIBS = -lcurses

all: tabs

tabs: tabs.o 
	$(CC) -o tabs tabs.o  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

tabs.o: tabs.c \
	$(INC)/stdio.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/types.h \
	$(INC)/stdlib.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/curses.h \
	$(INC)/term.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) tabs

clean:
	rm -f tabs.o

clobber: clean
	rm -f tabs

lintit:
	$(LINT) $(LINTFLAGS) tabs.c

#	These targets are useful but optional

partslist:
	@echo tabs.mk tabs.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo tabs | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit tabs.mk $(LOCALINCS) tabs.c -o tabs.o tabs
