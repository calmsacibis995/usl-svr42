#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nadmin.rfs:other/rfs/system/bin/system.mk	1.1.6.2"
#ident "$Header: system.mk 2.0 91/07/12 $"

include $(CMDRULES)

TARGETDIR =

LDLIBS = -lnsl

MAKEFILE = system.mk

MAINS = getaddr

OBJECTS =  getaddr.o

SOURCES =  getaddr.c

all : ALL

ALL:		$(MAINS)

getaddr:		getaddr.o
	$(CC) -o getaddr getaddr.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

getaddr.o:\
	$(INC)/stdio.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h \
	$(INC)/tiuser.h

install: all
	$(INS) -m 644 -g bin -u bin -f $(TARGETDIR) getaddr

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

size: ALL
	$(SIZE) $(MAINS)

strip: ALL
	$(STRIP) $(MAINS)

