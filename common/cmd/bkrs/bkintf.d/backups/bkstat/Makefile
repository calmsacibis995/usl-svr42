#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/bkstat/Makefile	1.1.8.4"
#ident "$Header: Makefile 1.4 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
BKSTATDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended/status
PRODUCTS=full limit modify select
OWN=bin
GRP=bin

rsstat: all

.sh:
	cp $< $*

O_HELPFILE = Help

all: $(O_HELPFILE) $(PRODUCTS)

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

install: all $(BKSTATDIR)
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(BKSTATDIR) $(O_HELPFILE)

size strip lintit:
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

$(BKSTATDIR):
	if [ ! -d `dirname $(BKSTATDIR)` ] ;\
	then \
		mkdir -p `dirname $(BKSTATDIR)` ;\
	fi
	if [ ! -d $(BKSTATDIR) ] ;\
	then \
		mkdir -p $(BKSTATDIR) ;\
	fi
