#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/devices/copy/Makefile	1.5.5.2"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(CMDRULES)

O_SHFILES= 
O_DFILES=\
	Help \
	copy.menu Form.diskette \
	Text.copy1 Text.copy2 Text.copy3 Text.cperr1 Text.cperr2 Text.cperr3

.MUTEX:	 all xinstall

all: $(O_SHFILES)

clean:

clobber: clean

size strip: all

lintit:

install: all xinstall

PROTO=Prototype
OBJECTS=$(O_SHFILES) $(O_DFILES)
include ../../Install
