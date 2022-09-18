#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)netsel.adm:netsel.adm.mk	1.2.7.4"
#ident  "$Header: netsel.adm.mk 2.1 91/07/26 $"

include $(CMDRULES)

OAMBASE=$(USRSADM)/sysadm
LOAMBASE=/usr/sadm/sysadm
PKGINST=nsu
PKGSAV=$(VAR)/sadm/pkg/$(PKGINST)/save
MENUDIR = $(OAMBASE)/menu/netservices/selection
INSDIR = $(OAMBASE)/add-ons/nsu/netservices/selection
LINSDIR = $(LOAMBASE)/add-ons/nsu/netservices/selection
BINDIR = $(OAMBASE)/bin
SHDIR = ./bin
LINK = ln

DIRS = $(MENUDIR) $(BINDIR) $(PKGSAV)/intf_install

OWN=bin
GRP=bin

O_SHFILES=nslist chgnetconf

display=Text.ns_list

modify=Menu.ns_nid Form.ns_modify Text.ns_modify

all:
	for x in $(O_SHFILES) ;\
	do \
		cp $(SHDIR)/$$x.sh $(SHDIR)/$$x;\
	done

clean:

clobber: clean
	for x in $(O_SHFILES) ;\
	do \
		rm -f $(SHDIR)/$$x ;\
	done

$(DIRS):
	- [ -d $@ ] || mkdir -p $@

install: all $(DIRS) 
	- [ -d $(INSDIR)/display ] || mkdir -p $(INSDIR)/display
	- [ -d $(INSDIR)/modify ] || mkdir -p $(INSDIR)/modify
# display
	for i in $(display) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(INSDIR)/display $$i ;\
	done
# modify
	for i in $(modify) ;\
	do \
		$(INS) -m 644 -g bin -u bin -f $(INSDIR)/modify $$i ;\
	done
	$(INS) -m 644 -g bin -u bin -f $(INSDIR) Help
	$(INS) -m 644 -g bin -u bin -f $(MENUDIR) selection.menu
	$(INS) -m 644 -g bin -u bin -f $(PKGSAV)/intf_install selection.mi
# link Help files
	-rm -f $(INSDIR)/display/Help $(MENUDIR)/Help $(INSDIR)/modify/Help
	$(LINK) $(INSDIR)/Help $(INSDIR)/display/Help
	$(LINK) $(INSDIR)/Help $(INSDIR)/modify/Help
	$(SYMLINK) $(LINSDIR)/Help $(MENUDIR)/Help 
#sh scripts
	for i in $(O_SHFILES) ;\
	do \
		$(INS) -m 755 -g bin -u bin -f $(BINDIR) $(SHDIR)/$$i ;\
	done

$(O_SHFILES):
	cp $(SHDIR)/$(@).sh $(SHDIR)/$(@)
	echo in sh

size:	all

strip:	all

lintit:

