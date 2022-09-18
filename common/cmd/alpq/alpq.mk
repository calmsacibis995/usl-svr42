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

#ident	"@(#)alpq:alpq.mk	1.1.2.1"
#ident "$Header: alpq.mk 1.4 91/07/02 $"

include $(CMDRULES)

DIR = $(USRBIN)

OFILES=	alpq.o

MAINS= alpq

all:	$(MAINS)

alpq:	$(OFILES)
	$(CC) $(OFILES) -o $(MAINS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install:	all
	$(INS) -f $(DIR) $(MAINS)

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(MAINS)
