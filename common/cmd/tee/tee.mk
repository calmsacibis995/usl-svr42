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

#ident	"@(#)tee:tee.mk	1.6.4.1"
#ident "$Header: tee.mk 1.2 91/03/15 $"

include $(CMDRULES)

#	Makefile for tee 

OWN = bin
GRP = bin

all: tee

tee: tee.o 
	$(CC) -o tee tee.o  $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

tee.o: tee.c \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) tee

clean:
	rm -f tee.o

clobber: clean
	rm -f tee

lintit:
	$(LINT) $(LINTFLAGS) tee.c

#	These targets are useful but optional

partslist:
	@echo tee.mk tee.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo tee | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit tee.mk $(LOCALINCS) tee.c -o tee.o tee
