#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cpiopc:cpiopc.mk	1.3.2.4"
#ident  "$Header: cpiopc.mk 1.1 91/07/03 $"

include $(CMDRULES)

#	Makefile for cpiopc

OWN = root
GRP = sys

LOCALDEF = -D_STYPES

all: .cpiopc

.cpiopc: cpiopc.o 
	$(CC) -o .cpiopc cpiopc.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

cpiopc.o: cpiopc.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/pwd.h 

install: all
	 $(INS) -f $(USRSBIN) -m 0755 -u $(OWN) -g $(GRP) .cpiopc

clean:
	rm -f cpiopc.o

clobber: clean
	rm -f .cpiopc

lintit:
	$(LINT) $(LINTFLAGS) cpiopc.c

#	These targets are useful but optional

partslist:
	@echo Makefile cpiopc.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo .cpiopc | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit Makefile $(GLOBALINCS) cpiopc.c -o cpiopc.o .cpiopc
