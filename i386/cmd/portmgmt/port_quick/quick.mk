#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:i386/cmd/portmgmt/port_quick/quick.mk	1.1.2.4"
#ident	"$Header: $"

include $(CMDRULES)

OAMBASE=$(USRSADM)/sysadm
INSDIR = $(OAMBASE)/menu/ports/port_quick
BINDIR = $(OAMBASE)/bin

SHFILES = q-add q-rm

CFILES = isastream

O_DFILES = Form.add Form.rm Menu.ap Menu.rp Text.cfa Text.cfr Text.priv quick.menu Help

all: isastream shells

isastream: isastream.c

shells:
		cp q-add.sh q-add
		cp q-rm.sh q-rm

clean:
	-rm -f *.o $(SHFILES) $(CFILES)

clobber: clean
	-rm -f $(SHFILES) $(CFILES)

install: all $(INSDIR) $(TASKS)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(INSDIR) $$i ;\
	done

	for i in $(SHFILES) ;\
	do \
		$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(BINDIR) $$i ;\
	done

	for i in $(CFILES) ;\
	do \
		$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(BINDIR) $$i ;\
	done

size: all

strip: all

lintit:


$(INSDIR):
	if [ ! -d $(INSDIR) ] ;\
	then \
		mkdir -p $(INSDIR) ;\
	fi
