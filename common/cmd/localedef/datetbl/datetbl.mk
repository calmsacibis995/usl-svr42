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

#ident	"@(#)localedef:common/cmd/localedef/datetbl/datetbl.mk	1.1.8.1"
#ident "$Header: datetbl.mk 1.2 91/04/17 $"

include $(CMDRULES)

#	Makefile for datetbl

OWN = bin
GRP = bin

all: $(MAINS)

install: all
	$(INS) -f $(USRLIB)/locale/C -m 0555 -u $(OWN) -g $(GRP) time_C
	$(INS) -f $(USRLIB)/locale/C LC_TIME
