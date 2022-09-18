#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/ln/ln.mk	1.1"
#ident	"$Header: $"
#       Portions Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved


#     Makefile for ln

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = ln.mk

MAINS = ln

OBJECTS =  ln.o

SOURCES = ln.c 

ALL:          $(MAINS)

$(MAINS):	ln.o
	$(CC) -o ln ln.o $(LDFLAGS) $(PERFLIBS)
	
ln.o:		$(INC)/stdio.h $(INC)/sys/types.h $(INC)/sys/param.h \
	$(INC)/sys/stat.h $(INC)/errno.h

GLOBALINCS = $(INC)/stdio.h $(INC)/sys/types $(INC)/sys/param.h \
	$(INC)/sys/stat.h $(INC)/errno.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all :	ALL

install:	ALL
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) ln 

