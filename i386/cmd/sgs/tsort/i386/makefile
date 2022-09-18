#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)tsort:i386/makefile	1.5"

include $(CMDRULES)


INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

SOURCES=$(COMDIR)/tsort.c

OBJECTS=tsort.o

PRODUCTS=tsort

all:	$(PRODUCTS)

$(PRODUCTS):	$(OBJECTS)
	$(CC) $(OBJECTS) $(LDLIBS) $(LINK_MODE) -o tsort

tsort.o:	$(COMDIR)/tsort.c
	$(CC) $(CFLAGS) -c $(COMDIR)/tsort.c

install:	all
	cp tsort tsort.bak
	$(STRIP) tsort
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)tsort tsort
	mv tsort.bak tsort

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:	
	-rm -f $(OBJECTS)

clobber:	clean
	-rm -f $(PRODUCTS)
