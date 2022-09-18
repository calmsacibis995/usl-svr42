#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/softwaremgmt/Makefile	1.4.8.4"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(CMDRULES)

DIRS=check defaults install interact list read_in remove

.MUTEX:	 all install
O_DFILES= \
	Menu.pkg software.menu pkg
HELPFILES=Help
HELPDEST=softwaremgmt/Help

all:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

clobber size strip clean lintit:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

install: xinstall 
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

PROTO=Prototype
OBJECTS=$(HELPFILES) $(O_DFILES)
OBJDEST=$(HELPDEST)
include ../Install
