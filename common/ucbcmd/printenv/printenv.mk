#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/printenv/printenv.mk	1.1"
#ident	"$Header: $"
# 	Copyright (c) 1988 Sun Microsystems, Inc.
#	All rights reserved.


#	Makefile for printenv 

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = printenv.mk

MAINS = printenv

OBJECTS =  printenv.o

SOURCES =  printenv.c

ALL:	$(MAINS)

printenv:	printenv.o 
	$(CC) -o printenv  printenv.o   $(LDFLAGS) $(PERFLIBS)


printenv.o:	$(INC)/stdio.h

GLOBALINCS = $(INC)/stdio.h 


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(OWN) $(MAINS)

