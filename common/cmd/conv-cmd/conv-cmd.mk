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

#ident	"@(#)conv-cmd:conv-cmd.mk	1.3.3.1"
#ident "$Header: conv-cmd.mk 1.2 91/03/22 $"
#
# conv-cmd.mk: makefile for 4.0 conversion commands
#

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = root
GRP = root

CFILES =\
	ttyconv

SHFILES =

PRODUCTS = ttyconv

all:		$(PRODUCTS)

ttyconv:	ttyconv.o
		$(CC) -o ttyconv ttyconv.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ttyconv

clean:
		-rm -f *.o
	
clobber:	clean
		-rm -f $(PRODUCTS)

FRC:
