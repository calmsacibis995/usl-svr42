#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)macinit:macinit.mk	1.9.2.2"
#ident "$Header: macinit.mk 1.2 91/04/17 $"

include $(CMDRULES)

#	Makefile for macinit

OWN = root
GRP = bin

ADMLIB = -ladm

MAINS   = macinit consalloc 
OBJECTS = macinit.o consalloc.o 
SOURCES = $(OBJECTS:.o=.c)

all: $(MAINS)

macinit: macinit.o
	$(CC) -o macinit macinit.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS) $(ADMLIB)

consalloc: consalloc.o
	$(CC) -o consalloc consalloc.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

consalloc.o: consalloc.c \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/stdio.h \
	$(INC)/mac.h $(INC)/sys/mac.h \
	$(INC)/errno.h $(INC)/sys/errno.h

macinit.o: macinit.c \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/uadmin.h \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/mac.h $(INC)/sys/mac.h

install: all
	-rm -f $(SBIN)/macinit
	$(INS) -f $(SBIN) -m 0500 -u $(OWN) -g $(GRP) macinit
	-rm -f $(SBIN)/consalloc
	$(INS) -f $(SBIN) -m 0500 -u $(OWN) -g $(GRP) consalloc

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

#	These targets are useful but optional

partslist:
	@echo macinit.mk \ $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(SBIN) | tr ' ' '\012' | sort

product:
	@echo \ | tr ' ' '\012' | \
	sed 's;^;$(SBIN)/;'
