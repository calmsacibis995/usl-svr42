#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/bksetup/summary/Makefile	1.3.7.4"
#ident "$Header: Makefile 1.4 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
BKBIN=$(SADMDIR)/bkup/bin
OAMBASE=$(SADMDIR)/sysadm
ETCBKUP=$(ETC)/bkup
SUMDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended/setup/summary
PRODUCT = summary
OWN=bin
GRP=bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.summary Text.summary Text.errs Help

all: $(O_SHFILES)

clean:

clobber: clean

install: all $(SUMDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(SUMDIR) $$i ;\
	done

size strip lintit:

$(SUMDIR):
	if [ ! -d `dirname $(SUMDIR)` ] ;\
	then \
		mkdir -p `dirname $(SUMDIR)` ;\
	fi
	if [ ! -d $(SUMDIR) ] ;\
	then \
		mkdir -p $(SUMDIR) ;\
	fi
