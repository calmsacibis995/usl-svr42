#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cb:cb.mk	1.9"

include $(CMDRULES)

LDLIBS=-lw
CMDBASE=../..

ENVPARMS= \
	CMDRULES="$(CMDRULES)" LDLIBS="$(LDLIBS)" CMDBASE="$(CMDBASE)"

all:
	cd $(CPU) ; $(MAKE) all $(ENVPARMS)

install:
	cd $(CPU) ; $(MAKE) install $(ENVPARMS)


lintit:
	cd $(CPU) ; $(MAKE) lintit $(ENVPARMS)

clean:
	cd $(CPU) ; $(MAKE) clean

clobber:
	cd $(CPU) ; $(MAKE) clobber
