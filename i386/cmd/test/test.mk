#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)test:test.mk	1.1.2.2"
#ident  "$Header: test.mk 1.1 91/05/17 $"

include $(CMDRULES)


OWN = bin
GRP = bin

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.

# Makefile for test.sh

# to install when not privileged
# set $(CH) in the environment to #

all: test

test: test.sh
	cp test.sh test

install: all
	 $(INS) -f $(USRBIN) -m 0775 -u $(OWN) -g $(GRP) test 

clean:

clobber: clean
	rm -f test
