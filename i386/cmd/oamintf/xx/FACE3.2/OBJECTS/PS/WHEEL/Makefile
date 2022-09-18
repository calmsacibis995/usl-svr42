#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/OBJECTS/PS/WHEEL/Makefile	1.2"
#ident	"$Header: $"

include $(CMDRULES)

MENUS=$(ROOT)/$(MACH)/usr/vmsys/OBJECTS/PS/WHEEL
MENUFILES=Form.asel Form.csel Form.display Form.dsel Form.wadd Form.wchg \
	  Menu.alert Menu.wdelete Menu.wheel Text.cadd Text.cadel Text.cchg \
	  Text.cdel Text.nchg Text.nmark Text.nodis Text.nowchg Text.nowheel
GRP=bin
OWN=bin


all: all
	@echo "nothing to build in OBJECTS/PS/WHEEL."

install:
	-[ -d $(MENUS) ] || mkdir -p $(MENUS)
	-for f in $(MENUFILES) ; \
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(MENUS) $$f ; \
	done

clean:
	@echo "nothing to clean in OBJECTS/PS/WHEEL."

clobber: clean
	@echo "nothing to clobber in OBJECTS/PS/WHEEL."

size strip lintit:
