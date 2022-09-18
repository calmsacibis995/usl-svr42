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

#ident	"@(#)clear:clear.mk	1.5.3.1"
#ident "$Header: clear.mk 1.2 91/03/21 $"
#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.


# Makefile for clear.sh

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

all: clear 

clear: clear.sh
	cp clear.sh clear

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) clear 

clean:

clobber:  clean
	rm -f clear 
