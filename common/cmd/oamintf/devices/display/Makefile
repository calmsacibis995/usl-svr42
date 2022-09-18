#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/devices/display/Makefile	1.5.7.2"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(CMDRULES)

O_SHFILES=\
	dispdisk
O_DFILES=\
	Help \
	Menu.display Form.display Text.display Text.disperr \
	Text.disprem

.MUTEX:	 all xinstall

all: $(O_SHFILES)

clean:

clobber: clean
	rm -f $(O_SHFILES)

lintit:

size strip: all

install: all xinstall

PROTO=Prototype
OBJECTS=$(O_SHFILES) $(O_DFILES)
include ../../Install
