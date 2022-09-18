#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rsintf.d/restores/Makefile	1.1.8.3"
#ident "$Header: Makefile 1.3 91/05/23 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
OAMBASE=$(SADMDIR)/sysadm
RESTOREDIR = $(OAMBASE)/add-ons/bkrs/restores/extended
SADMVAR=$(VAR)/sadm
PKGDIR=$(SADMVAR)/pkg/bkrs
MIDIR = $(PKGDIR)/save/intf_install
MIFILE = rest.mi
HELPFILE = Help
PRODUCTS=restore rsstat setup rsoper
OWN=bin
GRP=bin

restores: all

all: $(PRODUCTS) $(MIFILE) $(HELPFILE)

.sh:
	cp $< $*

$(PRODUCTS):
	cd $(@); $(MAKE) $(MAKEARGS) $(@); cd ..

$(MIFILE):
	grep -v "^#ident" 16122026388.mi >$(MIFILE)

clean:

clobber: clean
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done
	rm -f $(MIFILE)

install: all $(RESTOREDIR) $(PKGDIR) $(MIDIR)
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done

	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(MIDIR) $(MIFILE)
	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(RESTOREDIR) $(HELPFILE)

lintit:
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done

$(RESTOREDIR):
	if [ ! -d `dirname $(RESTOREDIR)` ] ;\
	then \
		mkdir -p `dirname $(RESTOREDIR)` ;\
	fi
	if [ ! -d $(RESTOREDIR) ] ;\
	then \
		mkdir -p $(RESTOREDIR) ;\
	fi

$(PKGDIR):
	if [ ! -d `dirname $(PKGDIR)` ] ;\
	then \
		mkdir -p `dirname $(PKGDIR)` ;\
	fi
	if [ ! -d $(PKGDIR) ] ;\
	then \
		mkdir -p $(PKGDIR) ;\
	fi

$(MIDIR):
	if [ ! -d `dirname $(MIDIR)` ] ;\
	then \
		mkdir -p `dirname $(MIDIR)` ;\
	fi
	if [ ! -d $(MIDIR) ] ;\
	then \
		mkdir -p $(MIDIR) ;\
	fi
