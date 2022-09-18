#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/OBJECTS/PS/FILTER/Makefile	1.2"
#ident	"$Header: $"

include $(CMDRULES)

MENUS=$(ROOT)/$(MACH)/usr/vmsys/OBJECTS/PS/FILTER
MENUFILES=Form.add Form.cd Form.change Form.fadd Menu.del Menu.filter \
	  Menu.input Menu.output Menu.pname Menu.ptype \
	  Text.addcfm Text.all Text.chgcfm Text.delcfm Text.display \
	  Text.nochg Text.nofltr Text.none
GRP=bin
OWN=bin

all:
	@echo "nothing to build in OBJECTS/PS/FILTER."

install: all
	-[ -d $(MENUS) ] || mkdir -p $(MENUS)
	-for f in $(MENUFILES) ; \
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(MENUS) $$f ; \
	done

clean:
	@echo "nothing to clean in OBJECTS/PS/FILTER."

clobber: clean
	@echo "nothing to clobber in OBJECTS/PS/FILTER."

size strip lintit:
