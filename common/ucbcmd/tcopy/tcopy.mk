#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/tcopy/tcopy.mk	1.1"
#ident	"$Header: $"


#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#     Makefile for tcopy

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

INC1 = $(ROOT)/$(MACH)/usr/ucbinclude

LOCALINC = -I$(INC1)

OWN = bin

GRP = bin

MAKEFILE = tcopy.mk

MAINS = tcopy

OBJECTS =  tcopy.o

SOURCES = tcopy.c 

ALL:          $(MAINS)

$(MAINS):	tcopy.o
	$(CC) -o tcopy tcopy.o $(LDFLAGS) $(PERFLIBS)
	
tcopy.o:		$(INC)/stdio.h \
		$(INC)/sys/file.h \
		$(INC)/sys/types.h \
		$(INC)/signal.h \
		$(INC)/sys/fcntl.h \
		$(INC1)/sys/mtio.h 


GLOBALINCS = $(INC)/stdio.h \
	$(INC)/sys/file.h \
	($INC)/sys/types.h \
	$(INC)/signal.h \
	$(INC)/sys/fcntl.h \
	$(INC1)/sys/mtio.h 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all :	ALL

install:	ALL
	$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) tcopy 

