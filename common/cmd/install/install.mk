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

#ident	"@(#)install:install.mk	1.2.6.2"
#ident "$Header: install.mk 1.3 91/06/17 $"

include $(CMDRULES)


OWN = bin
GRP = bin

all:
	cp install.sh install

install: all
	-rm -f $(ETC)/install
	 $(INS) -o -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) install
	-$(SYMLINK) /usr/sbin/install $(ETC)/install

clean:

clobber: clean
	-rm -rf install

lintit:
