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

#ident	"@(#)ttymap:ttymap.mk	1.3.2.2"
#ident "$Header: ttymap.mk 1.4 91/05/02 $"

include $(CMDRULES)

#	Makefile for ttymap

OWN = bin
GRP = bin

LOCALDEF=-DTTYMAP

all: ttymap

ttymap: ttymap.o 
	$(CC) -o ttymap ttymap.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ttymap.o: ttymap.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/ftw.h \
	$(INC)/ttymap.h \
	$(INC)/assert.h

install: all
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) ttymap

clean:
	rm -f ttymap.o

clobber: clean
	rm -f ttymap

lintit:
	$(LINT) $(LINTFLAGS) ttymap.c

#	These targets are useful but optional

partslist:
	@echo ttymap.mk ttymap.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo ttymap | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit ttymap.mk $(LOCALINCS) ttymap.c -o ttymap.o ttymap
