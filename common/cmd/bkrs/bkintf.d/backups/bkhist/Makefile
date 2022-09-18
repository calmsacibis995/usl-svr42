#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/bkhist/Makefile	1.7.8.4"
#ident "$Header: Makefile 1.4 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
BHISTDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended/history
PRODUCTS=full limit select
OWN=bin
GRP=bin

bkhist: all

O_SHFILES =

O_HELPFILE = Help

all: $(O_SHFILES) $(O_HELPFILE) $(PRODUCTS)

.sh:
	cp $< $*

$(PRODUCTS):
	cd $(@); $(MAKE) $(MAKEARGS) $(@); cd ..

clean:

clobber: clean
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done

size strip lintit:
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done

install: all $(BHISTDIR)
	for f in $(PRODUCTS) ;\
	do \
		cd $$f ;\
		$(MAKE) $(MAKEARGS) $(@) ;\
		cd .. ;\
	done

	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(BHISTDIR) $(O_HELPFILE)

$(BHISTDIR):
	if [ ! -d `dirname $(BHISTDIR)` ] ;\
	then \
		mkdir -p `dirname $(BHISTDIR)` ;\
	fi
	if [ ! -d $(BHISTDIR) ] ;\
	then \
		mkdir -p $(BHISTDIR) ;\
	fi
