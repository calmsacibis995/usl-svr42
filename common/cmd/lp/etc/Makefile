#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:etc/Makefile	1.11.2.2"
#ident "$Header: Makefile 1.2 91/04/12 $"
#
# Makefile for lp/etc
#


include $(CMDRULES)

TOP	=	..

include ../common.mk


INIT_D	=	$(ETC)/init.d
RC0_D	=	$(ETC)/rc0.d
RC2_D	=	$(ETC)/rc2.d

OWNER	=	root
GROUP	=	sys
RMODES	=	0444


all:

install:
	$(INS) -m $(RMODES) -u $(OWNER) -g $(GROUP) -f $(INIT_D) lp
	-$(LN) $(INIT_D)/lp $(RC0_D)/K20lp
	-$(LN) $(INIT_D)/lp $(RC2_D)/S80lp

clean:

clobber:

strip:

lintit:

lintlib:
