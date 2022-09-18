#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/backup/Makefile	1.7.8.4"
#ident "$Header: Makefile 1.4 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
BKUPDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended/backup
PRODUCT = backup
OWN=bin
GRP=bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.backup Text.bkpok Text.bkpnok Help

all: $(O_SHFILES)

clean:

clobber: clean

install: all $(BKUPDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(BKUPDIR) $$i ;\
	done

size strip lintit:

$(BKUPDIR):
	if [ ! -d `dirname $(BKUPDIR)` ] ;\
	then \
		mkdir -p `dirname $(BKUPDIR)` ;\
	fi
	if [ ! -d $(BKUPDIR) ] ;\
	then \
		mkdir -p $(BKUPDIR) ;\
	fi
