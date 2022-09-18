#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/bksetup/add/Makefile	1.5.7.4"
#ident "$Header: Makefile 1.4 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/etc/bkup
ADDDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended/setup/add
PRODUCT = add
OWN=bin
GRP=bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.add Form.add2 Text.addok Text.addnok Menu.methods Help

all: $(O_SHFILES)

clean:

clobber: clean

install: all $(ADDDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(ADDDIR) $$i ;\
	done

strip size lintit:

$(ADDDIR):
	if [ ! -d `dirname $(ADDDIR)` ] ;\
	then \
		mkdir -p `dirname $(ADDDIR)` ;\
	fi
	if [ ! -d $(ADDDIR) ] ;\
	then \
		mkdir -p $(ADDDIR) ;\
	fi
