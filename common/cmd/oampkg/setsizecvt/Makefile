#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)oampkg:common/cmd/oampkg/setsizecvt/Makefile	1.3"
#ident "$Header:$"

#	Makefile for setsizecvt

include $(CMDRULES)

INS = install

INSDIR = $(USRSADM)/install/bin

LOCALINC= -I../hdrs


MAKEFILE = Makefile

SOURCES =  setsizecvt.sh

all:		 $(SOURCES)
	cp setsizecvt.sh setsizecvt
	chmod 0755 setsizecvt

clean:
	rm -f setsizecvt
	: do nothing

clobber:
	: do nothing

install: all
	$(INS) -f $(INSDIR) -m 0755 -u bin -g bin setsizecvt 
