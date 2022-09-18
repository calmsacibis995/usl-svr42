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

#ident	"@(#)uniq:uniq.mk	1.2.6.1"
#ident "$Header: uniq.mk 1.3 91/03/19 $"

include $(CMDRULES)

#	Makefile for uniq

OWN = bin
GRP = bin

LDLIBS = -lw

all: uniq

uniq: uniq.c
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) uniq

clean:
	rm -f uniq.o

clobber: clean
	rm -f uniq

lintit:
	$(LINT) $(LINTFLAGS) uniq.c
