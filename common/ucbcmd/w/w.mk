#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/w/w.mk	1.3"
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

#	Makefile for w/uptime

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = root

GRP = bin

MAKEFILE = w.mk

MAINS = w uptime

OBJECTS =  w.o

SOURCES =  w.c

ALL:		$(MAINS)

w: w.o
	$(CC) -o w  w.o $(LDFLAGS) $(PERFLIBS)

uptime:	w
	@/bin/ln w uptime



w.o:	$(INC)/stdio.h $(INC)/fcntl.h $(INC)/time.h $(INC)/sys/types.h \
	$(INC)/sys/types.h $(INC)/sys/param.h $(INC)/utmp.h \
	$(INC)/sys/utsname.h $(INC)/sys/stat.h $(INC)/dirent.h \
	$(INC)/sys/procfs.h $(INC)/sys/proc.h

GLOBALINCS = $(INC)/stdio.h $(INC)/fcntl.h $(INC)/time.h $(INC)/sys/types.h \
        $(INC)/sys/types.h $(INC)/sys/param.h $(INC)/utmp.h \
        $(INC)/sys/utsname.h $(INC)/sys/stat.h $(INC)/dirent.h \
        $(INC)/sys/procfs.h $(INC)/sys/proc.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)


all : ALL

install: ALL
	$(INS) -m 555 -u $(OWN) -g $(GRP) -f $(INSDIR) w 
	rm -f $(INSDIR)/uptime
	/bin/ln $(INSDIR)/w $(INSDIR)/uptime

