#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)peruser:peruser.mk	1.2"

include	$(CMDRULES)

MAX_LIM		= 1000
DFLT_LEVEL	= 2
LOCALDEF	= -DMAX_LIM=$(MAX_LIM) -DDFLT_LEVEL=$(DFLT_LEVEL)


all:		peruser

install:	all
	$(INS) -f $(USRLIB) peruser

peruser.o:	peruser.c

peruser:	peruser.o
	$(CC) $(LDFLAGS) -o $@ peruser.o -lld

clean:
	-rm -rf *.o

clobber:	clean
	-rm -rf peruser
