#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/devices/Makefile	1.2"
#ident "$Header: Makefile 1.1 91/08/29 $"

include $(CMDRULES)

DIRS=add copy format erase display remove

O_SHFILES= \
	getdlst showgrp valdrive getdisk valdisk ckdisk devtabcmd listtypes \
	getdevice valdevice
O_DFILES= \
	Help \
	Text.diskerr Text.nodev Menu.disk Menu.drive Menu.device devices.menu \
	Text.rsvrd Text.rsvrd2

.MUTEX: all xinstall

all:	$(O_SHFILES)
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

lintit size strip clean:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

clobber: clean
	rm -f $(O_SHFILES)
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f Makefile $(MAKEARGS) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f Makefile $(MAKEARGS) $@ ;\
			cd .. ;\
		fi ;\
	done

install: $(O_SHFILES) xinstall 
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
OBJECTS=$(O_DFILES) $(O_SHFILES)
include ../Install
