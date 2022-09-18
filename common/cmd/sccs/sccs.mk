#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sccs:sccs.mk	6.7.4.3"

include $(CMDRULES)

HELPLIB=$(CCSLIB)/help

ENVPARAMS = CMDRULES="$(CMDRULES)" HELPLIB="$(HELPLIB)"

.MUTEX: libs cmds helplib
all: libs cmds helplib 
	@echo "SCCS is built"

lintit: 
	cd lib; $(MAKE) -f lib.mk $(ENVPARAMS) lintit
	cd cmd; $(MAKE) -f cmd.mk $(ENVPARAMS) lintit
	@echo "SCCS is linted"

libs:
	cd lib; $(MAKE) -f lib.mk $(ENVPARAMS)

cmds:
	cd cmd; $(MAKE) -f cmd.mk $(ENVPARAMS)

helplib:
	cd help.d; $(MAKE) -f help.mk $(ENVPARAMS)

install:
	cd lib; $(MAKE) -f lib.mk $(ENVPARAMS) install
	cd cmd; $(MAKE) -f cmd.mk $(ENVPARAMS) $(ARGS) install
	cd help.d; $(MAKE) -f help.mk $(ENVPARAMS) install

clean:
	cd lib; $(MAKE) -f lib.mk $(ENVPARAMS) clean
	cd cmd; $(MAKE) -f cmd.mk $(ENVPARAMS) clean
	cd help.d; $(MAKE) -f help.mk $(ENVPARAMS) clean

clobber:
	cd lib; $(MAKE) -f lib.mk $(ENVPARAMS) clobber
	cd cmd; $(MAKE) -f cmd.mk $(ENVPARAMS) clobber
	cd help.d; $(MAKE) -f help.mk $(ENVPARAMS) clobber 
