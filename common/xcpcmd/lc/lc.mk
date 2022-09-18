#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)xcplc:lc.mk	1.2.2.1"
#ident  "$Header: lc.mk 1.3 91/07/26 $"

include $(CMDRULES)

#	Makefile for lc

all: lc

lc: 

install: all
	-rm -f $(USRBIN)/lc
	ln $(USRBIN)/ls $(USRBIN)/lc 

clean clobber lintit:
