#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:filter/Makefile	1.15.2.5"
#ident "$Header: Makefile 1.3 91/04/18 $"
#
# Makefile for lp/filter
#



include $(CMDRULES)

TOP	=	..

include ../common.mk


MISC	= \
		slow.filter \
		pcl


all:		$(MISC)
	cd postscript; $(MAKE) $(MAKEARGS) all
	cd pcldir; $(MAKE) $(MAKEARGS) all

install:	all
	for misc in $(MISC); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(LPBINDIR) $$misc; \
	done
	cd postscript; $(MAKE) $(MAKEARGS) install
	cd pcldir; $(MAKE) $(MAKEARGS) install

clean clobber:
	cd postscript; $(MAKE) $(MAKEARGS) $@
	cd pcldir; $(MAKE) $(MAKEARGS) $@

pcl:
	cp pcldir/pcl.filter pcl

strip:

lintit:

lintlib:
