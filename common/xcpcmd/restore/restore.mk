#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcprestore:common/xcpcmd/restore/restore.mk	1.1.2.2"
#ident  "$Header: restore.mk 1.2 91/07/11 $"

include $(CMDRULES)

#	Makefile for restore

OWN = 
GRP = 

all: restore

install: all
	$(INS) -f $(USRBIN) restore 
	-ln $(USRBIN)/restore $(USRBIN)/.restore

clean:

clobber: clean
	rm -f restore

lintit:

#	These targets are useful but optional

partslist:
	@echo restore.mk restore.sh $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo restore | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit restore.mk $(LOCALINCS) restore.sh -o $(OBJECTS) restore
