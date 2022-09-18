#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:cmd/lpsched/Makefile	1.10.9.2"
#ident "$Header: Makefile 1.4 91/04/18 $"
#
# Makefile for LP scheduler trio
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk


DEBUG	=

##########
#
# If you add directories other than one level deep
# you'll have to change the code below.
##########
CMDDIRS= \
		./lpNet \
		./lpsched


all:		cmds

install clean clobber:
	for dir in $(CMDDIRS); \
	do \
		cd $$dir; \
		$(MAKE) $(MAKEARGS) $@; \
		cd ..; \
	done

strip:
	for dir in $(CMDDIRS); \
	do \
		cd $$dir; \
		$(MAKE) STRIP=$(STRIP) $(MAKEARGS) strip; \
		cd ..; \
	done

cmds:
	for dir in $(CMDDIRS); \
	do \
		cd $$dir; \
		$(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)"; \
		cd ..; \
	done

lintit:
	for dir in $(CMDDIRS); \
	do \
		cd $$dir; \
		$(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)" LINT="$(LINT)" lintit; \
		cd ..; \
	done

lintlib:
	for dir in $(CMDDIRS); \
	do \
		cd $$dir; \
		$(MAKE) $(MAKEARGS) DEBUG="$(DEBUG)" LINT="$(LINT)" lintlib; \
		cd ..; \
	done
