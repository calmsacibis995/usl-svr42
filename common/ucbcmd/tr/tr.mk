#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/tr/tr.mk	1.1"
#ident	"$Header: $"
# 	Portions Copyright (c) 1988, Sun Microsystems, Inc.
#	All rights reserved.


#	Makefile for tr 

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = tr.mk

MAINS = tr

OBJECTS =  tr.o

SOURCES =  tr.c

ALL:	$(MAINS)

tr:	tr.o 
	$(CC) -o tr  tr.o   $(LDFLAGS) $(PERFLIBS)


tr.o:	$(INC)/stdio.h

GLOBALINCS = $(INC)/stdio.h 


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) $(MAINS)

