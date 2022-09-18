#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/users/users.mk	1.1"
#ident	"$Header: $"
#	Copyright (c) 1988, Sun Microsystems, Inc.
#	All rights reserved.


#	Makefile for users 

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

MAKEFILE = users.mk

OWN = bin

GRP = bin

MAINS = users

OBJECTS =  users.o

SOURCES =  users.c

ALL:		$(MAINS)

users:		users.o 
	$(CC) -o users  users.o   $(LDFLAGS) $(PERFLIBS)


users.o:	 $(INC)/stdio.h $(INC)/utmp.h $(INC)/sys/types.h

GLOBALINCS = $(INC)/stdio.h $(INC)/utmp.h $(INC)/sys/types.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) users

