#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)comm:comm.mk	1.3.6.2"
#ident "$Header: comm.mk 1.2 91/03/22 $"
#	Makefile for comm

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

MAKEFILE = comm.mk

MAINS = comm

OBJECTS =  comm.o

SOURCES =  comm.c

all:		$(MAINS)

comm:	$(SOURCES)
	$(CC) $(CFLAGS) $(DEFLIST) -o comm comm.c $(LDFLAGS) $(LDLIBS) \
		$(PERFLIBS)

strip:
	$(STRIP) $(MAINS)
clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)


install: all
	$(INS) -f $(INSDIR)  -m 0555 -u $(OWN) -g $(GRP) $(MAINS)
