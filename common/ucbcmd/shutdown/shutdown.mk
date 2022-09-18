#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/shutdown/shutdown.mk	1.2"
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

#     Makefile for shutdown

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

LOCALINC = -I$(ROOT)/$(MACH)/usr/ucbinclude

INC1 = $(ROOT)/$(MACH)/usr/ucbinclude

LIB_OPT = -L $(ROOT)/$(MACH)/usr/ucblib

LDLIBS = -s -lnsl -lsocket $(LIB_OPT) -lucb

MAKEFILE = shutdown.mk

MAINS = shutdown

OBJECTS =  shutdown.o mountxdr.o

SOURCES = shutdown.c mountxdr.c

ALL:          $(MAINS)

$(MAINS):	shutdown.o mountxdr.o
	$(CC) -o shutdown shutdown.o mountxdr.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)
	
shutdown.o:		$(INC)/stdio.h \
		$(INC1)/sys/reboot.h \
		$(INC)/errno.h \
		$(INC)/signal.h \
		$(INC)/pwd.h \
		$(INC)/sys/types.h \
		$(INC)/sys/time.h

GLOBALINCS =	$(INC)/stdio.h \
		$(INC1)/sys/reboot.h \
		$(INC)/errno.h \
		$(INC)/signal.h \
		$(INC)/pwd.h \
		$(INC)/sys/types.h \
		$(INC)/sys/time.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all :	ALL

install:	ALL
	$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) shutdown 

