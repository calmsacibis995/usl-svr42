#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/bkrem/add/Makefile	1.4.7.5"
#ident "$Header: Makefile 1.4 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
ADDDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended/reminder/add
PRODUCT = add
OWN=bin
GRP=bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.add Text.addok Text.addnok Help

all: $(O_SHFILES)

clean:

clobber: clean

size strip lintit:

install: all $(ADDDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(ADDDIR) $$i ;\
	done

$(ADDDIR):
	if [ ! -d `dirname $(ADDDIR)` ] ;\
	then \
		mkdir -p `dirname $(ADDDIR)` ;\
	fi
	if [ ! -d $(ADDDIR) ] ;\
	then \
		mkdir -p $(ADDDIR) ;\
	fi
