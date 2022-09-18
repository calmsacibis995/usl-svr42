#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/lint/lint.mk	1.1"
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

#	Makefile for BSD Compatibility Package lint

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

MAKEFILE = lint.mk

MAINS = lint 

SOURCES =  lint.sh 

ALL:	$(MAINS)

lint:	lint.sh 
	cp lint.sh lint

clobber:
	rm -f $(MAINS)

clean:

all : ALL

install: ALL
	$(INS) -f $(INSDIR) -m 0555 $(MAINS) 

