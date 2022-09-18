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

#ident	"@(#)printf:printf.mk	1.3.4.1"
#ident "$Header: printf.mk 1.2 91/04/16 $"

include $(CMDRULES)

#	Makefile for printf

OWN = bin
GRP = bin

LDLIBS = -lgen

all: printf

printf: printf.o 
	$(CC) -o printf printf.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

printf.o: printf.c \
	$(INC)/stdio.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) printf

clean:
	rm -f printf.o

clobber: clean
	rm -f printf

lintit:
	$(LINT) $(LINTFLAGS) printf.c

#	These targets are useful but optional

partslist:
	@echo printf.mk printf.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo printf | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit printf.mk $(LOCALINCS) printf.c -o printf.o printf
