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

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.	 

#ident	"@(#)xcplibx:common/xcplib/libx/port/makefile	1.2.3.1"
#ident  "$Header: makefile 1.2 91/07/04 $"

include $(LIBRULES)

#
# makefile for libx/port/sys
#
#

OWN = 
GRP = 

.SUFFIXES: .p

.MUTEX: prof nonprof

LOCALDEF = -DMERGE
PROF =
NONPROF =

OBJECTS = sdget.o lock.o

POBJECTS = $(OBJECTS:.o=.p)


all:	prof nonprof

nonprof: $(OBJECTS)

prof: $(POBJECTS)

lock.o: lock.c \
	$(INC)/sys/lock.h

sdget.o: sdget.c \
	$(INC)/sys/sd.h \
	$(INC)/errno.h

.c.o:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(SDEFLIST) $(CFLAGS) -c $*.c

.c.p:
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(CFLAGS) -c -p $*.c && mv $(*F).o $*.p

clean:
	-rm -f $(OBJECTS) $(POBJECTS)

clobber: clean

lintit:
	$(LINT) $(LINTFLAGS) $(OBJECTS:.o=.c)

