#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/nametoaddr.mk	1.6.5.2"
#ident "$Header: nametoaddr.mk 1.3 91/03/15 $"
#
# makefile for name to address mapping dynamic linking libraries.
#

include $(LIBRULES)

TESTDIR = .
FRC =

all:
	@for i in * ; do  \
		if [ -d $$i ] ; then \
			cd $$i;\
			echo "##### $(MAKE) -f $$i.mk";\
			$(MAKE) -f $$i.mk $(MAKEARGS);\
			cd ..; \
		fi; \
	done;

install: 
	@for i in * ; do  \
		if [ -d $$i ] ; then \
			cd $$i;\
			echo $$i;\
			echo "##### $(MAKE) install -f $$i.mk";\
			$(MAKE) install -f $$i.mk $(MAKEARGS);\
			cd ..;\
		fi; \
	done;

clean:
	@for i in * ; do  \
		if [ -d $$i ] ; then \
			cd $$i;\
			echo $$i;\
			$(MAKE) -f $$i.mk clean $(MAKEARGS);\
			cd .. ;\
		fi; \
	done

clobber:
	@for i in * ; do  \
		if [ -d $$i ] ; then \
			cd $$i;\
			echo $$i;\
			$(MAKE) -f $$i.mk clobber $(MAKEARGS);\
			cd .. ;\
		fi; \
	done

lintit:

FRC:
