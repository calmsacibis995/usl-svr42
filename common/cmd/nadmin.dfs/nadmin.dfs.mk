#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nadmin.dfs:nadmin.dfs.mk	1.5.6.2"
#ident "$Header: nadmin.dfs.mk 2.0 91/07/12 $"

#Makefile for generic portions of distributed file systems

include $(CMDRULES)

OAMBASE=$(USRSADM)/sysadm
TARGETDIR = $(OAMBASE)/menu/netservices/remote_files
NETDIR = $(OAMBASE)/menu/netservices
REMOTEDIR = $(OAMBASE)/menu/netservices/remote_files
MDIR=../nadmin.dfs.mk

all:

install: all
	if [ ! -d $(REMOTEDIR) ] ; then \
		mkdir -p $(REMOTEDIR) ;\
	fi ;\
	for i in *; do\
		if [ -d $$i ]; then\
			if [ ! -d $(TARGETDIR)/$$i ]; then\
			mkdir -p $(TARGETDIR)/$$i;\
			fi;\
			cd $$i;\
			echo "make directory $$i";\
			$(MAKE) install $(MAKEARGS) "TARGETDIR=$(TARGETDIR)/$$i" "MDIR=../$(MDIR)" -f $(MDIR) ;\
			cd ..;\
		else if  [ $$i = "netserve.menu" ] ; then\
			$(INS) -m 644 -g bin -u bin -f $(NETDIR) $$i;\
			else if  [ $$i = "Help.netserve" ] ; then\
				$(INS) -m 644 -g bin -u bin -f $(NETDIR) $$i;\
				mv $(NETDIR)/Help.netserve $(NETDIR)/Help; \
				else if [ $$i != "nadmin.dfs.mk" ] ; then \
					echo "installing $$i";\
					$(INS) -m 644 -g bin -u bin -f $(TARGETDIR)  $$i;\
				fi;\
			fi;\
		fi;\
		fi;\
	done

clean:

clobber: clean

lintit:

