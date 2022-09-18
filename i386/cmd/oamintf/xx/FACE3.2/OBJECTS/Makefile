#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/OBJECTS/Makefile	1.2"
#ident	"$Header: $"

include $(CMDRULES)

MENUS=$(ROOT)/$(MACH)/usr/vmsys/OBJECTS
MENUDIRS=backup dskopts fs mailset hwset PS restore logins
MENUFILES=Menu.admin Menu.appsetup Text.h Menu.h0.toc Text.help Menu.programs \
	  Text.rsterr
GRP=bin
OWN=bin

all:
	@echo "nothing to build in VMSYS/OBJECTS."

install: all
	-[ -d $(MENUS) ] || mkdir -p $(MENUS)
	-for f in $(MENUFILES) ; \
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(MENUS) $$f ; \
	done

	-for dir in $(MENUDIRS) ; \
	do \
		( cd $$dir ; $(MAKE) $(MAKEARGS) $@ ) ; \
	done

clean:
	@echo "nothing to clean in VMSYS/OBJECTS."

clobber: clean
	@echo "nothing to clobber in VMSYS/OBJECTS."

size strip lintit:
