#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/whoami/whoami.mk	1.1"
#ident	"$Header: $"
#	Copyright (c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved.


#	Makefile for whoami 

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = whoami.mk

MAINS = whoami

OBJECTS =  whoami.o

SOURCES =  whoami.c

ALL:		$(MAINS)

whoami:	whoami.o
	$(CC) -o whoami whoami.o $(LDFLAGS) $(PERFLIBS)

whoami.o:	 $(INC)/pwd.h

GLOBALINCS = $(INC)/pwd.h 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) $(MAINS)

