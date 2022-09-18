#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cb:i386/makefile	1.3"
#
#	CB MAKEFILE
#
#

include $(CMDRULES)

LINT_CMD=$(LINT) $(USRINC)

INSDIR=$(CCSBIN)
LDLIBS=-lw
CMDBASE=../..
INS=$(CMDBASE)/install/install.sh
SGSBASE=../../sgs
COMDIR=../common
CPUINC=$(SGSBASE)/inc/$(CPU)
HFILE= $(CPUINC)/sgs.h
INCLIST= -I$(CPUINC) -I$(INC)

CFILE = $(COMDIR)/cb.c
PRODUCTS=cb

all:	$(PRODUCTS)

$(PRODUCTS):	cb.o
	$(CC) cb.o $(LINK_MODE) -o $(PRODUCTS) $(LDLIBS)

cb.o:	$(CFILE) $(HFILE)
	$(CC) $(CFLAGS) $(INCLIST) -c $(CFILE) 


install: all
	 cp cb cb.bak
	 $(STRIP) cb
	 /bin/sh $(INS) -f $(INSDIR) cb
	 mv cb.bak cb

clean:
	 -rm -f *.o

clobber: clean
	-rm -f cb

lintit:	$(HFILE) $(CFILE)
	$(LINT_CMD) $(INCLIST) $(CFILE)
