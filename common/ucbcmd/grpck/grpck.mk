#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/grpck/grpck.mk	1.1"
#ident	"$Header: $"
#	Portions Copyright (c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved.


#	Makefile for grpck

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = grpck.mk

MAINS = grpck

OBJECTS =  grpck.o

SOURCES =  grpck.c

ALL:		$(MAINS)

$(MAINS):	grpck.o
		$(CC) -o grpck grpck.o $(LDFLAGS) $(PERFLIBS)

grpck.o:	 $(INC)/stdio.h \
		 $(INC)/ctype.h \
		 $(INC)/pwd.h 

GLOBALINCS = $(INC)/ctype.h \
	$(INC)/pwd.h \
	$(INC)/stdio.h 


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) $(MAINS) 

