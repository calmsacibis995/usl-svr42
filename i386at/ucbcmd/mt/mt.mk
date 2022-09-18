#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:i386at/ucbcmd/mt/mt.mk	1.1"
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

#     Makefile for mt

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

INC1 = $(ROOT)/$(MACH)/usr/ucbinclude

LOCALDEF = -Dsun

LOCALINC = -I$(INC1) -I.

OWN = bin

GRP = bin

MAKEFILE = mt.mk

MAINS = mt

OBJECTS =  mt.o

SOURCES = mt.c 

ALL:          $(MAINS)

$(MAINS):	mt.o
	$(CC) -o mt mt.o $(LDFLAGS) $(PERFLIBS)

mt.o:		$(INC1)/stdio.h \
		$(INC)/ctype.h \
		$(INC1)/sys/types.h \
		$(INC1)/sys/mtio.h \
		$(INC)/sys/ioctl.h \
		$(INC1)/sys/param.h \
		$(INC)/sys/buf.h \
		$(INC1)/sys/file.h \
		$(INC)/sys/conf.h \
		$(INC)/sys/uio.h \
		$(INC)/sys/errno.h


#GLOBALINCS = $(INC)/stdio.h \ 
#	$(INC)/ctype.h \
#	$(INC)/sys/types.h \
#	$(INC)/sys/mtio.h \
#	$(INC)/sys/param.h \
#	$(INC2)/sys/ioctl.h \
#	$(INC)/sys/buf.h \
#	$(INC)/sys/file.h \
#	$(INC)/sys/conf.h \
#	$(INC)/sys/uio.h \
#	$(INC)/sys/errno.h
#
#
clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all :	ALL

install:	ALL
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) mt 

