#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/sched/Makefile	1.2"
#ident	"$Header: $"

include $(CMDRULES)

OAMBASE=$(USRSADM)/sysadm
VMSYS=$(USR)/vmsys/OBJECTS
FILEDIR = $(OAMBASE)/menu/sched

TASKS = sched


sched = Form.add Form.chg Menu.chg Menu.day Menu.del Menu.hour Menu.minute Menu.month Menu.sched Menu.week Text.addcfm Text.chgcfm Text.delcfm Text.display Text.noaces Text.nocron Text.nodel Text.notask Help

all: install


clean:

clobber:

install: $(FILEDIR) 
# sched
	-for i in $(sched) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(FILEDIR) $$i ;\
	done
	-ln $(FILEDIR)/Menu.sched $(VMSYS)/sched/Menu.sched

$(FILEDIR):
	-[ -d $(FILEDIR) ] || mkdir -p $(FILEDIR)
	-[ -d $(VMSYS)/sched ] || mkdir -p $(VMSYS)/sched
