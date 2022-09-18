#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:cmd/cmd.mk	1.3.2.2"

#       Makefile for cmd directory

PACKAGE_SCRIPTS=make_flops ask_drive mini_kernel
ALL=readfloppy $(PACKAGE_SCRIPTS)

all: $(ALL) 

install: $(ALL)
	cp $? ..

clean:
	rm -f *.o

clobber: clean
	rm -f $(ALL)
