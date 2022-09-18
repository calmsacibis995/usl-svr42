#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)basename:basename.mk	1.3.6.1"
#ident	"$Header: $"

include $(CMDRULES)
OWN=bin
GRP=bin
INSDIR=$(USRBIN)

all:	basename.sh
	cp basename.sh  basename

install:	all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) basename

clean:

clobber:	clean
	rm -f basename
