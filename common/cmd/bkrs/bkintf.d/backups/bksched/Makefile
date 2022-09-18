#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/bksched/Makefile	1.7.8.4"
#ident "$Header: Makefile 1.4 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
BKSCHEDDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended/schedule
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

install: all $(BKSCHEDDIR)
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(BKSCHEDDIR) $(O_HELPFILE)

size strip lintit:
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

$(BKSCHEDDIR):
	if [ ! -d `dirname $(BKSCHEDDIR)` ] ;\
	then \
		mkdir -p `dirname $(BKSCHEDDIR)` ;\
	fi
	if [ ! -d $(BKSCHEDDIR) ] ;\
	then \
		mkdir -p $(BKSCHEDDIR) ;\
	fi
