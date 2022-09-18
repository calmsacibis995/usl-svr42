#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:crontab/Makefile	1.9.2.2"
#ident "$Header: Makefile 1.2 91/04/12 $"

#
# Makefile for lp/crontab
#

include $(CMDRULES)

CRONTAB	=	$(VAR)/spool/cron/crontabs

OWNER	=	root
GROUP	=	root
MODE	=	444

SRC	=	lp

all:

install:
	$(INS) -m $(MODE) -u $(OWNER) -g $(GROUP) -f $(CRONTAB) $(SRC)

clobber:

clean:

strip:

lintit:

lintlib:
