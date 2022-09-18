#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/WHEEL/Makefile	1.2"
#ident	"$Header: $"

# Shell files do not lose their .sh suffix in this directory.

include $(CMDRULES)

BINDIR=$(ROOT)/$(MACH)/usr/vmsys/admin/PS/WHEEL
BINARY=
SHELLS=addwheel.sh avlfm.sh getall.sh getwheel.sh mkwheel.sh nowheel.sh \
	rmwheel.sh vcmount.sh
PRODUCTS=$(BINARY) $(SHELLS)
GRP=bin
OWN=bin


all:
	@echo "***** Making VMSYS/admin/PS/WHEEL"
	@echo "nothing to build."

install: all
	-[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	-for f in $(PRODUCTS) ; \
	do \
		$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(BINDIR) $$f ; \
	done

clean:
	@echo "nothing to clean in VMSYS/admin/PS/WHEEL."

clobber: clean
	@echo "nothing to clobber in VMSYS/admin/PS/WHEEL."

size strip lintit:
