#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rsintf.d/restores/setup/Makefile	1.7.7.3"
#ident "$Header: Makefile 1.3 91/05/23 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
OAMBASE=$(SADMDIR)/sysadm
SETDIR = $(OAMBASE)/add-ons/bkrs/restores/extended/operator
PRODUCT = setup
OWN=bin
GRP=bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.setup Text.setok Text.setnok Help

all: $(O_SHFILES)

clean:

clobber: clean

install: all $(SETDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(SETDIR) $$i ;\
	done

size strip lintit:

$(SETDIR):
	if [ ! -d `dirname $(SETDIR)` ] ;\
	then \
		mkdir -p `dirname $(SETDIR)` ;\
	fi
	if [ ! -d $(SETDIR) ] ;\
	then \
		mkdir -p $(SETDIR) ;\
	fi
