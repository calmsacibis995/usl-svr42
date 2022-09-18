#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/softwaremgmt/defaults/Makefile	1.2.8.2"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(CMDRULES)

O_SHFILES=
O_DFILES=\
	Form.sd_add Form.sd_list Form.sd_mod Form.sd_remove \
	defaults.menu Menu.logins Text.sd_addok Text.sd_addnok \
	Text.sd_modok Text.sd_modnok Text.sd_remok Text.sd_remnok
HELPFILES=\
	Help
HELPDEST=\
	defaults/Help

.MUTEX:	 all xinstall

all: $(O_SHFILES)

clean:
	:

clobber: clean
	rm -f $(O_SHFILES)

size strip: all
	:

lintit:

install: all xinstall

PROTO=../Prototype
OBJECTS=$(HELPFILES) $(O_SHFILES) $(O_DFILES)
OBJDEST=$(HELPDEST)
include ../../Install
