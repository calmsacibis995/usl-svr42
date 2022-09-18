#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rsintf.d/restores/rsstat/Makefile	1.9.7.3"
#ident "$Header: Makefile 1.3 91/05/23 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
OAMBASE=$(SADMDIR)/sysadm
RSSTATDIR = $(OAMBASE)/add-ons/bkrs/restores/extended/status
PRODUCTS=full modify select
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


size strip lintit:
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

install: all $(RSSTATDIR)
	for f in $(PRODUCTS) ; \
	do \
		cd $$f; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(RSSTATDIR) $(O_HELPFILE)

$(RSSTATDIR):
	if [ ! -d `dirname $(RSSTATDIR)` ] ;\
	then \
		mkdir `dirname $(RSSTATDIR)` ;\
	fi
	if [ ! -d $(RSSTATDIR) ] ;\
	then \
		mkdir $(RSSTATDIR) ;\
	fi
