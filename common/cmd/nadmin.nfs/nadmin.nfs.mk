#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nadmin.nfs:nadmin.nfs.mk	1.4.4.2"
#ident "$Header: nadmin.nfs.mk 2.0 91/07/12 $"
#makefile for nfs administration screens

include $(CMDRULES)

OAMBASE=$(USRSADM)/sysadm
TARGETDIR = $(OAMBASE)/menu/netservices/remote_files
MDIR = ../nadmin.nfs.mk

all:

install: all
	for i in * ; do \
		if [ -d $$i ] ; then \
			if [ ! -d $(TARGETDIR)/$$i ] ; then \
			mkdir -p $(TARGETDIR)/$$i  ;\
			fi ; \
			cd $$i ;\
			$(MAKE) install $(MAKEARGS) "TARGETDIR=$(TARGETDIR)/$$i" "MDIR=../$(MDIR)" -f $(MDIR);\
			cd .. ;\
		else \
			if [ $$i != "nadmin.nfs.mk" ] ;\
			then \
				echo "installing $$i" ;\
				$(INS) -m 644 -g bin -u bin -f $(TARGETDIR)  $$i ;\
			fi ;\
		fi  ;\
	done

clean:

clobber: clean

lintit:
