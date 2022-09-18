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

#ident	"@(#)tr:tr.mk	1.3.5.1"
#ident "$Header: tr.mk 1.4 91/04/29 $"

include $(CMDRULES)

OWN = bin
GRP = bin

LDLIBS = -lw

OBJECTS = tr.o mtr.o

SOURCES = tr.c mtr.c

all: tr

tr: $(OBJECTS)
	$(CC) -o tr $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

tr.o: tr.c \
	$(INC)/stdio.h \
	$(INC)/sys/euc.h \
	$(INC)/getwidth.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

mtr.o: mtr.c \
	$(INC)/stdio.h \
	$(INC)/sys/euc.h \
	$(INC)/pfmt.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) tr

clean:
	rm -f $(OBJECTS)
	
clobber: clean
	rm -f tr

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

# 	These targets are useful but optional

remove:
	cd $(USRBIN); rm -f tr

partslist:
	@echo tr.mk $(LOCALINCS) $(SOURCES) | tr ' ' '\012' | sort

product:
	@echo tr | tr ' ' '\012' | \
	sed -e 's;^;$(USRBIN)/;' -e 's;//*;/;g'

productdir:
	@echo $(USRBIN)
