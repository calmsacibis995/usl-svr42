#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/Makefile	1.3.9.4"
#ident "$Header: Makefile 1.4 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
BACKUPDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended
SADMVAR=$(VAR)/sadm
PKGDIR=$(SADMVAR)/pkg/bkrs
MIDIR = $(PKGDIR)/save/intf_install
MIFILE = bkup.mi
HELPFILE = Help
PRODUCTS=backup bkhist bkoper bkrem bksched bksetup bkstat
OWN=bin
GRP=bin

backups: all

all: $(PRODUCTS) $(MIFILE) $(HELPFILE)

.sh:
	cp $< $*

$(PRODUCTS):
	cd $(@); $(MAKE) $(MAKEARGS) $(@); cd ..

$(MIFILE):
	grep -v "^#ident" 08455426588.mi >$(MIFILE)

clean:

clobber: clean
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done
	rm -f $(MIFILE)

size strip lintit:
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done

install: all $(BACKUPDIR) $(PKGDIR) $(MIDIR)
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done

	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(MIDIR) $(MIFILE)
	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(BACKUPDIR) $(HELPFILE)

$(BACKUPDIR):
	if [ ! -d `dirname $(BACKUPDIR)` ] ;\
	then \
		mkdir -p `dirname $(BACKUPDIR)` ;\
	fi
	if [ ! -d $(BACKUPDIR) ] ;\
	then \
		mkdir -p $(BACKUPDIR) ;\
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
