#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/tset/tset.mk	1.1"
#ident	"$Header: $"
#       Portions Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved


#     Makefile for tset

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

LDLIBS = -lcurses -ltermlib 

OWN = bin

GRP = bin

MAKEFILE = tset.mk

MAINS = tset 

OBJECTS =  tset.o

SOURCES = tset.c 

ALL:          $(MAINS)

$(MAINS):	tset.o
	$(CC) -o tset tset.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)
	
tset.o:		$(INC)/stdio.h 

GLOBALINCS = $(INC)/stdio.h 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all :	ALL

install:	ALL
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) tset 
	rm -f $(INSDIR)/reset
	ln $(INSDIR)/tset $(INSDIR)/reset
	
