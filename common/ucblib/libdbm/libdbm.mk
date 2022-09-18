#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucblib/libdbm/libdbm.mk	1.1"
#ident	"$Header: $"

include $(LIBRULES)

MAKEFILE = libdbm.mk

INSDIR=$(ROOT)/$(MACH)/usr/ucblib

INC1=$(ROOT)/$(MACH)/usr/ucbinclude

INCSYS=$(INC)/sys

ARFLAGS = rv

LOCALINC = -I$(INC1)

LIBRARY = libdbm.a

OBJECTS =  dbm.o

SOURCES =  dbm.c

ALL:		$(LIBRARY)

all:	ALL

$(LIBRARY):	dbm.o
		$(AR) $(ARFLAGS) $(LIBRARY) dbm.o


dbm.o:		 $(INCSYS)/select.h \
		 $(INCSYS)/stat.h \
		 $(INCSYS)/types.h \
		 $(INC1)/dbm.h 

GLOBALINCS = $(INCSYS)/select.h \
	$(INCSYS)/stat.h \
	$(INCSYS)/types.h \
	$(INC1)/dbm.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(LIBRARY)

install: ALL
	$(INS) -f $(INSDIR) -m 644 $(LIBRARY)
