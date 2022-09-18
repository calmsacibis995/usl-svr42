#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libmalloc:libmalloc.mk	1.27"

include $(LIBRULES)

.SUFFIXES: .p

ARFLAGS = r
LIBP= $(CCSLIB)/libp
LINTFLAGS=-u -m -lmalloc
SOURCES=malloc.c
OBJECTS=malloc.o
POBJECTS=malloc.p
FRC=

INS=$(CMDBASE)/install/install.sh
INSDIR=$(CCSBIN)

LINK_MODE=


all:
	cd $(CPU) ; $(MAKE) all

install:
	cd $(CPU) ; $(MAKE) install

lintit:
	cd $(CPU) ; $(MAKE) lintit

clean:
	cd $(CPU) ; $(MAKE) clean

clobber:
	cd $(CPU) ; $(MAKE) clobber
