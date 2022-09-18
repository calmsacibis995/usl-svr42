#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/Makefile	1.3"
#ident	"$Header: $"

# Note: In 3.2 FACE, some shells install as .sh and others do not!

include $(CMDRULES)

BINDIR=$(ROOT)/$(MACH)/usr/vmsys/admin
OAMBIN=$(USRSADM)/sysadm/bin
DOTFILE=chkuser
OTHBINARY=chkuser
BINARY=
SHELLS=
PRODUCTS=$(BINARY) $(SHELLS)
SUBDIR=PS
GRP=bin
OWN=bin

all: $(PRODUCTS) $(OTHBINARY)

$(OTHBINARY):
	@echo "***** Making VMSYS/admin"
	$(CC) $(CFLAGS) $(DEFLIST) $(@).c -o $(@) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	-[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	-[ -d $(OAMBIN) ] || mkdir -p $(OAMBIN)

	-for f in $(DOTFILE) ; \
	do \
		cp $$f .$$f ; \
	done

	$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(BINDIR) .chkuser
	-rm -f $(OAMBIN)/.chkuser
	-ln $(BINDIR)/.chkuser $(OAMBIN)/.chkuser

	-for d in $(SUBDIR) ; \
	do \
		( cd $$d ; $(MAKE) $(MAKEARGS) $(@) ) ; \
	done

clean clobber:
	@echo "***** Cleaning/Clobbering VMSYS/admin"
	rm -f *.o $(PRODUCTS) $(OTHBINARY) .$(DOTFILE) lint.out
	-for d in $(SUBDIR) ; \
	do \
		( cd $$d ; $(MAKE) $(MAKEARGS) $(@) ) ; \
	done

size:
	$(SIZE) $(OTHBINARY)
	-for d in $(SUBDIR) ; \
	do \
		( cd $$d ; $(MAKE) $(MAKEARGS) $(@) ) ; \
	done

strip:
	$(STRIP) $(OTHBINARY)
	-for d in $(SUBDIR) ; \
	do \
		( cd $$d ; $(MAKE) $(MAKEARGS) $(@) ) ; \
	done

lintit:
	$(LINT) $(LINTFLAGS) $(DEFLIST) $(OTHBINARY).c $(LINTLIBS) >> lint.out
	-for d in $(SUBDIR) ; \
	do \
		( cd $$d ; $(MAKE) $(MAKEARGS) $(@) ) ; \
	done
