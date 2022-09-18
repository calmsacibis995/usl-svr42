#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cscope:cscope.mk	1.21"

include $(CMDRULES)

ENVPARMS= \
	CMDRULES="$(CMDRULES)" 

all:
	cd $(CPU) ; $(MAKE) all

install:
	cd $(CPU) ; $(MAKE) install ROOT=$(ROOT)

lintit:
	cd $(CPU) ; $(MAKE) lintit $(ENVPARMS)

clean:
	cd $(CPU) ; $(MAKE) clean

clobber:
	cd $(CPU) ; $(MAKE) clobber
