#ident	"@(#)ucb:common/ucbcmd/unifdef/unifdef.mk	1.2"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.




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

#     Makefile for unifdef

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = unifdef.mk

MAINS = unifdef

OBJECTS =  unifdef.o

SOURCES = unifdef.c 

ALL:          $(MAINS)

$(MAINS):	unifdef.o
	$(CC) -o unifdef unifdef.o $(LDFLAGS) $(PERFLIBS)
	
unifdef.o:		$(INC)/stdio.h \
		$(INC)/ctype.h 

GLOBALINCS = $(INC)/stdio.h \
	$(INC)/ctype.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all :	ALL

install:	ALL
	$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) unifdef 

