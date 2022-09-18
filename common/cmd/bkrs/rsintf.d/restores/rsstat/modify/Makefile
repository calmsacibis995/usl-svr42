#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rsintf.d/restores/rsstat/modify/Makefile	1.4.7.3"
#ident "$Header: Makefile 1.3 91/05/23 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
OAMBASE=$(SADMDIR)/sysadm
CHGDIR = $(OAMBASE)/add-ons/bkrs/restores/extended/status/modify
PRODUCT = modify
OWN=bin
GRP=bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.modify Text.modok Text.modnok Help

all: $(O_SHFILES)

clean:

clobber: clean

install: all $(CHGDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(CHGDIR) $$i ;\
	done

size strip lintit:

$(CHGDIR):
	if [ ! -d `dirname $(CHGDIR)` ] ;\
	then \
		mkdir -p `dirname $(CHGDIR)` ;\
	fi
	if [ ! -d $(CHGDIR) ] ;\
	then \
		mkdir -p $(CHGDIR) ;\
	fi
