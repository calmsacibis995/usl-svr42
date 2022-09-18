#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/bkintf.d/backups/bksetup/except/remove/Makefile	1.3.9.4"
#ident	"$Header: Makefile 1.5 91/09/11 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
OAMBASE=$(SADMDIR)/sysadm
REMDIR = $(OAMBASE)/add-ons/bkrs/backup_service/extended/setup/exception_list/remove
PRODUCT = remove
OWN=bin
GRP=bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.remove Text.remove Help

all: $(O_SHFILES)

clean:

clobber: clean

install: all $(REMDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(REMDIR) $$i ;\
	done

size strip lintit:

$(REMDIR):
	if [ ! -d `dirname $(REMDIR)` ] ;\
	then \
		mkdir -p `dirname $(REMDIR)` ;\
	fi
	if [ ! -d $(REMDIR) ] ;\
	then \
		mkdir -p $(REMDIR) ;\
	fi
