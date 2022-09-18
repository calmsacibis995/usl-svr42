#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/ps/ps.mk	1.4"
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

#       Makefile for ps in BSD Compatiblity Package

include $(CMDRULES)

LDLIBS = -lw 

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = root

GRP = sys

all:	ps

ps:	ps.o
	$(CC) -o ps ps.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

ps.o:	ps.c

install: ps
	$(INS) -f $(INSDIR) -m 4755 -u $(OWN) -g $(GRP) ps

clean:
	-rm -f ps.o

clobber: clean
	rm -f ps
