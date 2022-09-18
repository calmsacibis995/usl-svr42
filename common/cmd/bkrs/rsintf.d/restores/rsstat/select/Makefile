#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rsintf.d/restores/rsstat/select/Makefile	1.5.7.3"
#ident "$Header: Makefile 1.3 91/05/23 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
OAMBASE=$(SADMDIR)/sysadm
SELDIR = $(OAMBASE)/add-ons/bkrs/restores/extended/status/selective
PRODUCT = select
OWN=bin
GRP=bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.select Text.select Text.errs Help

all: $(O_SHFILES)

clean:

clobber: clean

install: all $(SELDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(SELDIR) $$i ;\
	done

size strip lintit:

$(SELDIR):
	if [ ! -d `dirname $(SELDIR)` ] ;\
	then \
		mkdir -p `dirname $(SELDIR)` ;\
	fi
	if [ ! -d $(SELDIR) ] ;\
	then \
		mkdir -p $(SELDIR) ;\
	fi
