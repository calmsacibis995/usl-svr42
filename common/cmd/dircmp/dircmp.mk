#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dircmp:dircmp.mk	1.2.3.1"
#ident "$Header: dircmp.mk 1.2 91/04/05 $"
#	Makefile for dircmp

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

MAKEFILE = dircmp.mk

MAINS = dircmp

OBJECTS =  dircmp

SOURCES =  dircmp.sh

all:		$(MAINS)

dircmp:	 
	cp dircmp.sh dircmp

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)


install: all
	$(INS) -f $(INSDIR)  -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

