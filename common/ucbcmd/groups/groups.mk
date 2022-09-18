#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/groups/groups.mk	1.1"
#ident	"$Header: $"
#     Portions Copyright(c) 1988, Sun Microsystems, Inc.
#     All Rights Reserved


#     Makefile for groups

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = groups.mk

MAINS = groups

OBJECTS =  groups.o

SOURCES = groups.c 

ALL:          $(MAINS)

$(MAINS):	groups.o
	$(CC) -o groups groups.o $(LDFLAGS) $(PERFLIBS)
	
groups.o:		$(INC)/stdio.h $(INC)/sys/param.h \
			$(INC)/pwd.h	$(INC)/grp.h

GLOBALINCS = $(INC)/stdio.h $(INC)/sys/param.h \
		$(INC)/pwd.h	$(INC)/grp.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all :	ALL

install:	ALL
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 00555 $(MAINS)

