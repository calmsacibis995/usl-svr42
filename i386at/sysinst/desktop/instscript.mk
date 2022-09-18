#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/instscript.mk	1.7.1.5"
#ident	"$Header: $"

ROOT=/
STRIP=strip
SBUS=wd7000
INC=$(ROOT)/usr/include

default:
	echo "Nothing specified"

all: install

install:
	-cd menus; (for j in * ; \
                        do \
                           [ -d ../ifiles/$$j ] || mkdir ../ifiles/$$j; \
                           cd $$j ;\
                           for i in * ; \
                           do  egrep -v "^#" $$i > ../../ifiles/$$j/$$i;\
                           done ; \
                           cd .. ; \
                        done )
