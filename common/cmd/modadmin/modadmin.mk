#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)modadmin:modadmin.mk	1.2"
#ident	"$Header: $"

include $(CMDRULES)

#	Makefile for modadmin

OWN = root
GRP = sys

LDLIBS = -lgen

all:	modadmin

modadmin:	modadmin.o
	$(CC) -o modadmin modadmin.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

modadmin.o:	modadmin.c		\
		$(INC)/libgen.h		\
		$(INC)/locale.h		\
		$(INC)/pfmt.h		\
		$(INC)/stdio.h		\
		$(INC)/stdlib.h		\
		$(INC)/string.h		\
		$(INC)/sys/errno.h	\
		$(INC)/sys/mod.h	\
		$(INC)/time.h

clean:
	rm -f modadmin.o

clobber: clean
	rm -f modadmin

lintit:
	$(LINT) $(LINTFLAGS) modadmin.c

install:	all
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) modadmin
