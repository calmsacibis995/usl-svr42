#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/oam/oam.mk	1.6.5.3"
#ident  "$Header: oam.mk 1.4 91/06/13 $"

#makefile for name to address administration screens

include $(LIBRULES)

TARGETDIR = $(USRSADM)/sysadm/menu/netservices
MDIR = ../oam.mk

OWN=bin
GRP=bin

all:

install: all
	@for i in * ; do \
		if [ -d $$i ] ; then \
			[ -d $(TARGETDIR)/$$i ] || mkdir -p $(TARGETDIR)/$$i ;\
			echo `pwd`: ;\
			cd $$i ;\
			$(MAKE) -i "TARGETDIR=$(TARGETDIR)/$$i" \
			     "MDIR=../$(MDIR)" -f $(MDIR) $(MAKEARGS) install ;\
			cd .. ;\
		else \
			if [ $$i != "oam.mk" ] ;\
			then \
			if [ `basename $(TARGETDIR)` = "HELP" ] ; then \
				grep -v \"^#ident\" $(TARGETDIR)/$$i >/usr/tmp/$$i;\
				mv /usr/tmp/$$i $(TARGETDIR)/$$i;\
				echo "installing $$i";\
				$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(TARGETDIR) $$i;\
			else \
				echo "installing $$i" ;\
				$(INS) -m 755 -g $(GRP) -u $(OWN) -f $(TARGETDIR)  $$i ;\
			fi ;\
			fi ;\
		fi  ;\
	done
			
clean:

clobber: clean

lintit:
