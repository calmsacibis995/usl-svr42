#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/devices/remove/Makefile	1.2"
#ident "$Header: Makefile 1.1 91/08/28 $"

include $(CMDRULES)

O_DFILES=\
	Help \
	Menu.remove Form.remove Text.remove Text.rmnok Text.rmok

.MUTEX:	 all xinstall

all:

clean:

clobber: clean

lintit:

size strip: all


install: all xinstall

PROTO=Prototype
OBJECTS=$(O_DFILES)
include ../../Install
