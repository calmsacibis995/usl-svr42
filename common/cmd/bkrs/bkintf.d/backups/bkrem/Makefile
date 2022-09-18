#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/bkrem/Makefile	1.3.7.4"
#ident "$Header: Makefile 1.4 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
BKMSGDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended/reminder
PRODUCTS=add display modify remove
OWN=bin
GRP=bin

rsstat: all

.sh:
	cp $< $*

O_HELPFILE = Help

all: $(O_SHFILES) $(O_HELPFILE) $(PRODUCTS)

$(PRODUCTS):
	cd $(@); $(MAKE) $(MAKEARGS) $(@); cd ..

clean:

clobber: clean
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

size strip lintit:
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

install: all $(BKMSGDIR)
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(BKMSGDIR) $(O_HELPFILE)

$(BKMSGDIR):
	if [ ! -d `dirname $(BKMSGDIR)` ] ;\
	then \
		mkdir -p `dirname $(BKMSGDIR)` ;\
	fi
	if [ ! -d $(BKMSGDIR) ] ;\
	then \
		mkdir -p $(BKMSGDIR) ;\
	fi
