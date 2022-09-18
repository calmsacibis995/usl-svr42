#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/bkhist/select/Makefile	1.6.7.4"
#ident "$Header: Makefile 1.4 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
SELDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended/history/selective
PRODUCT = select
OWN=bin
GRP=bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.select Text.select Text.bkherr Text.bkhok Form.oldfile Help

all: $(O_SHFILES)

size strip lintit:

clean:

clobber: clean

install: all $(SELDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(SELDIR) $$i ;\
	done

$(SELDIR):
	if [ ! -d `dirname $(SELDIR)` ] ;\
	then \
		mkdir -p `dirname $(SELDIR)` ;\
	fi
	if [ ! -d $(SELDIR) ] ;\
	then \
		mkdir -p $(SELDIR) ;\
	fi
