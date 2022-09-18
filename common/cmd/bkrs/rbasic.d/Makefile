#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rbasic.d/Makefile	1.1"
#ident	"$Header: $"

include $(CMDRULES)

OAMBASE=$(ROOT)/$(MACH)/usr/sadm/sysadm
BOPERDIR = $(OAMBASE)/menu/restores/restore/basic
PRODUCT = rbasic
GRP = bin
OWN = bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Help Menu.restore Text.privil

P_FILES=Form.overwr Form.overwr2 Menu.device Menu.device2 Menu.private Menu.select
S_FILES=Form.overwr Form.overwr2 Menu.device Menu.device2 Menu.system Menu.select
all: $(O_SHFILES)

size strip lintit clean clobber:

install: all $(BOPERDIR) BOPERDIRSUB
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(BOPERDIR) $$i ;\
	done

	for i in $(P_FILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(BOPERDIR)/private  private/$$i ;\
	done

	for i in $(S_FILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(BOPERDIR)/system system/$$i ;\
	done


$(BOPERDIR):
	if [ ! -d `dirname $(BOPERDIR)` ] ;\
	then \
		mkdir -p `dirname $(BOPERDIR)` ;\
	fi
	if [ ! -d $(BOPERDIR) ] ;\
	then \
		mkdir -p $(BOPERDIR) ;\
	fi

BOPERDIRSUB:
	if [ ! -d `dirname $(BOPERDIR)/private` ] ;\
	then \
		mkdir -p `dirname $(BOPERDIR)/private` ;\
	fi
	if [ ! -d $(BOPERDIR)/private ] ;\
	then \
		mkdir -p $(BOPERDIR)/private ;\
	fi

	if [ ! -d `dirname $(BOPERDIR)/system` ] ;\
	then \
		mkdir -p `dirname $(BOPERDIR)/system` ;\
	fi
	if [ ! -d $(BOPERDIR)/system ] ;\
	then \
		mkdir -p $(BOPERDIR)/system ;\
	fi
