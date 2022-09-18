#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/straddr/straddr.mk	1.1.7.3"
#ident "$Header: straddr.mk 1.3 91/03/15 $"

#	Makefile for straddr.so

include $(LIBRULES)

LIBNAME=	straddr.so
OBJECTS=	straddr.o

LOCALDEF=-DPIC $(PICFLAG)
LOCALLDFLAGS=-s -dy -G -ztext -h /usr/lib/$(LIBNAME)

all:		$(LIBNAME)

$(LIBNAME):	$(OBJECTS)
		$(CC) $(LOCALLDFLAGS) -o $(LIBNAME) $(OBJECTS)


straddr.o:	$(INC)/stdio.h $(INC)/tiuser.h $(INC)/netdir.h \
			$(INC)/netconfig.h $(INC)/ctype.h straddr.c

clean:
		rm -f $(OBJECTS)

clobber:
		rm -f $(OBJECTS) $(LIBNAME)

install:	all
		$(INS) -f $(USRLIB) $(LIBNAME)

size:		all
		$(SIZE) $(LIBNAME)

strip:		all
		$(STRIP) $(LIBNAME)

lintit:
