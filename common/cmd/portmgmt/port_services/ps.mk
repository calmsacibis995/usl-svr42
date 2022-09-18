#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:common/cmd/portmgmt/port_services/ps.mk	1.6.7.2"
#ident "$Header: ps.mk 2.2 91/08/12 $"

include $(CMDRULES)

OAMBASE=$(USRSADM)/sysadm
INSDIR = $(OAMBASE)/menu/ports/port_services
BINDIR = $(OAMBASE)/bin

MAINS=pmckmod
PMCKMOD=pmckmod.c
PMCKMODOBJ=pmckmod.o
PMCKMOD=pmckmod

TASKS = add disable enable list modify remove

O_DFILES = ps.menu Menu.c_labels Help

add=Menu.ps_add Form.ps_add Menu.ps_atag Menu.ps_atype Form.ps_addls \
	Form.ps_addtm Form.ps_adduk Text.ps_msg

disable=Menu.ps_disabl

enable=Menu.ps_enable

list=Menu.ps_list Menu.ps_lstag Menu.ps_lstype Text.ps_list Text.ps_lsall

modify=Menu.ps_modify Form.ps_modls Form.ps_modtm 

remove=Menu.ps_remove

all: $(MAINS)

$(PMCKMOD): $(PMCKMODOBJ)
	$(CC) -o $(PMCKMOD) $(PMCKMODOBJ) $(SHLIBS)

clean:
	-rm -f *.o

clobber: clean
	-rm -f $(MAINS)
	
install: all dirs
	$(INS) -m 755 -g bin -u bin -f $(BINDIR) $(PMCKMOD)
# add
	for i in $(add) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(INSDIR)/add $$i ;\
	done
# disable
	for i in $(disable) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(INSDIR)/disable $$i ;\
	done
# enable
	for i in $(enable) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(INSDIR)/enable $$i ;\
	done
# list
	for i in $(list) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(INSDIR)/list $$i ;\
	done
# modify
	for i in $(modify) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(INSDIR)/modify $$i ;\
	done
# remove
	for i in $(remove) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(INSDIR)/remove $$i ;\
	done
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(INSDIR) $$i ;\
	done
#symbolic link all Help files
	-ln $(INSDIR)/Help $(INSDIR)/add/Help
	-ln $(INSDIR)/Help $(INSDIR)/disable/Help
	-ln $(INSDIR)/Help $(INSDIR)/enable/Help
	-ln $(INSDIR)/Help $(INSDIR)/list/Help
	-ln $(INSDIR)/Help $(INSDIR)/modify/Help
	-ln $(INSDIR)/Help $(INSDIR)/remove/Help

size: all

strip: all

dirs:
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	for dir in $(TASKS) ;\
	do \
		[ -d $(INSDIR)/$$dir ] || mkdir -p $(INSDIR)/$$dir ;\
	done
