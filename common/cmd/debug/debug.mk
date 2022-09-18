#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:debug.mk	1.2"

# The "all" target builds both the command line interface
# and the graphical user interface"
# The "cli" and "gui" targets build one or the other

include $(CMDRULES)
include util/common/defs.make

all:
	$(MAKE) -f debugsrc.mk all $(MAKEARGS)

cli:
	$(MAKE) -f debugsrc.mk cli $(MAKEARGS)

gui:
	$(MAKE) -f debugsrc.mk gui $(MAKEARGS)

install:
	$(MAKE) -f debugsrc.mk install $(MAKEARGS)

install_cli:
	$(MAKE) -f debugsrc.mk install_cli $(MAKEARGS)

install_gui:
	$(MAKE) -f debugsrc.mk install_cli $(MAKEARGS)

lintit:
	$(MAKE) -f debugsrc.mk lintit $(MAKEARGS)

clean:
	$(MAKE) -f debugsrc.mk clean $(MAKEARGS)

clobber:
	$(MAKE) -f debugsrc.mk clobber $(MAKEARGS)

