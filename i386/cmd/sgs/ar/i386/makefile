#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ar:i386/makefile	1.6"
#***************************************************************************
#                                                                          *
#			UNIX Archive (ar) makefile			   *
#									   *
#***************************************************************************

include $(CMDRULES)

INS=$(SGSBASE)/sgs.install
INSDIR=$(CCSBIN)

ARFORMAT = PORTAR

HFILES = $(CPUINC)/sgs.h $(CPUINC)/paths.h $(COMINC)/libelf.h

SOURCES=$(COMDIR)/ar.c

OBJECTS=ar.o

PRODUCTS=ar

DEFLIST	= -D$(ARFORMAT)
INCLIST	= -I$(COMINC) -I$(CPUINC)
CC_CMD=$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)

all:	$(PRODUCTS)


$(PRODUCTS):	$(OBJECTS) $(LIBELF)
	$(CC) $(OBJECTS) $(LINK_MODE) $(LIBELF) $(LIBSGS) -o ar

ar.o:	$(HFILES) $(COMDIR)/ar.c
	$(CC_CMD) -c $(COMDIR)/ar.c

install: all	
	cp ar ar.bak
	$(STRIP) ar
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(INSDIR)/$(SGS)ar ar
	mv ar.bak ar

lintit: $(SOURCES) $(HFILES)
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(INCLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f $(PRODUCTS)
