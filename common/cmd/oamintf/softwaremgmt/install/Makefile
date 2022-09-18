#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/softwaremgmt/install/Makefile	1.2.6.2"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(CMDRULES)

O_SHFILES=
O_DFILES=\
	Form.install
HELPFILES=\
	Help
HELPDEST=\
	install/Help

.MUTEX:	 all install

all: $(O_SHFILES)

clean:
	:

clobber: clean
	rm -f $(O_SHFILES)

lintit:
	:

size strip: all
	:

install: all xinstall

PROTO=../Prototype
OBJECTS=$(HELPFILES) $(O_SHFILES) $(O_DFILES)
OBJDEST=$(HELPDEST)
include ../../Install
