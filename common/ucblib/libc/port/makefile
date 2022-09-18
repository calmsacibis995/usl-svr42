#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucblib/libc/port/makefile	1.2"
#ident	"$Header: $"
#
# makefile for ucblibc/port
#
#

include $(LIBRULES)

PROF=
NONPROF=
SDEFLIST=

all:
	cd gen; $(MAKE)  $(IGN)
	cd stdio; $(MAKE)  $(IGN)
	cd sys; $(MAKE)  $(IGN)

clean:

clobber: clean
	-rm -f */*.o
