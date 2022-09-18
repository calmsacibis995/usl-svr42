#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sdb:sdb.mk	1.31"

include $(CMDRULES)

all:
	$(MAKE) -f sdbsrc.mk all $(MAKEARGS)

install:
	$(MAKE) -f sdbsrc.mk install $(MAKEARGS)

lintit:
	$(MAKE) -f sdbsrc.mk lintit $(MAKEARGS)

clean:
	$(MAKE) -f sdbsrc.mk clean $(MAKEARGS)

clobber:
	$(MAKE) -f sdbsrc.mk clobber $(MAKEARGS)

