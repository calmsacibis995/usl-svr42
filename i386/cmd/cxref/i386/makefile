#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cxref:i386/makefile	1.5"

include $(CMDRULES)

LIBELF=
LDLIBS= -lw
LINK_MODE=
LINTFLAGS=
CPUINC=../../sgs/inc/$(CPU)
INS=../../install/install.sh
INSDIR=$(CCSBIN)
COMDIR=../common
SOURCES=$(COMDIR)/cxref.c $(COMDIR)/st.c
OBJECTS=cxref.o st.o
PRODUCTS=xref

all:		$(PRODUCTS)

xref:		$(OBJECTS)
		$(CC) $(OBJECTS) $(LINK_MODE) $(LDLIBS) -o xref

cxref.o:	$(COMDIR)/cxref.c $(COMDIR)/cxref.h
		$(CC) $(CFLAGS) -I$(CPUINC) -I$(INC) -c $(COMDIR)/cxref.c

st.o:		$(COMDIR)/st.c
		$(CC) $(CFLAGS) -I$(INC) -c $(COMDIR)/st.c

install:	all
		cp xref xref.bak
		$(STRIP) xref
		cp $(COMDIR)/cxref.sh cxref
		/bin/sh $(INS) -f $(CCSLIB) xref
		/bin/sh $(INS) -f $(INSDIR) cxref
		rm -f cxref
		mv xref.bak xref

lintit:		$(SOURCES)
		$(LINT) $(LINTFLAGS) -I$(CPUINC) -I$(INC) $(SOURCES)

clean:
		-rm -f $(OBJECTS)

clobber:	clean
		-rm -f $(PRODUCTS)
