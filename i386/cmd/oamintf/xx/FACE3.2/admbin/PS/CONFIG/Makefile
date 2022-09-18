#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/CONFIG/Makefile	1.2"
#ident	"$Header: $"

include $(CMDRULES)

BINDIR=$(ROOT)/$(MACH)/usr/vmsys/admin/PS/CONFIG
BINARY=alphanum
SHELLS=
PRODUCTS=$(BINARY) $(SHELLS)
GRP=bin
OWN=bin

all: $(PRODUCTS)

$(BINARY):
	@echo "***** Making VMSYS/admin/PS/CONFIG"
	$(CC) $(CFLAGS) $(DEFLIST) $(@).c -o $(@) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	-[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	-for f in $(PRODUCTS) ; \
	do \
		$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(BINDIR) $$f ; \
	done

clean:
	@echo "***** Cleaning VMSYS/admin/PS/CONFIG"
	rm -f *.o lint.out

clobber: clean
	@echo "***** Clobbering VMSYS/admin/PS/CONFIG"
	rm -f $(PRODUCTS)

strip:
	$(STRIP) $(BINARY)

size:
	$(SIZE) $(BINARY)

lintit:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(BINARY).c $(LINTLIBS) >> lint.out
