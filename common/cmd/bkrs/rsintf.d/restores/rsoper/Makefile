#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rsintf.d/restores/rsoper/Makefile	1.5.7.3"
#ident "$Header: Makefile 1.3 91/05/23 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
OAMBASE=$(SADMDIR)/sysadm
RSODIR = $(OAMBASE)/add-ons/bkrs/restores/extended/respond
PRODUCT = respond
OWN=bin
GRP=bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.rsoper Menu.methods Text.rsoper Text.err Help

all: $(O_SHFILES)

clean:

clobber: clean

size strip lintit:

install: all $(RSODIR)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(RSODIR) $$i ;\
	done

$(RSODIR):
	if [ ! -d `dirname $(RSODIR)` ] ;\
	then \
		mkdir -p `dirname $(RSODIR)` ;\
	fi
	if [ ! -d $(RSODIR) ] ;\
	then \
		mkdir -p $(RSODIR) ;\
	fi
