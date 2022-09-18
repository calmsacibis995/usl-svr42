#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libdl:i386/makefile	1.1"
#
# i386-specific makefile for libdl
#
#

include $(LIBRULES)

SGSBASE=../../../cmd/sgs
INS=$(SGSBASE)/sgs.install
INSDIR=
HFILES=
SOURCES=
OBJECTS= dlclose.o dlerror.o dlopen.o dlsym.o
PRODUCTS=
DEFLIST=

#
all: libdl.so
	mv libdl.so ../libdl.so

libdl.so:	$(OBJECTS)
	$(AR) -r libdl.so $(OBJECTS)

dlclose.o:	dlclose.s
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(INCLIST) dlclose.s

dlerror.o:	dlerror.s
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(INCLIST) dlerror.s

dlopen.o:	dlopen.s
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(INCLIST) dlopen.s

dlsym.o:	dlsym.s
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(INCLIST) dlsym.s

install:	all
	/bin/sh $(INS) 755 $(OWN) $(GRP) $(CCSLIB)/libdl.so ../libdl.so

clean:
	rm -f */*.o

clobber:	clean
	rm -f libdl.so ../libdl.so
