#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcpyes:yes.mk	1.2.2.2"
#ident  "$Header: yes.mk 1.2 91/07/11 $"

include $(CMDRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved
#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.
# Makefile for yes
# to install when not privileged
# set $(CH) in the environment to #

OWN = bin
GRP = bin

INSDIR = $(USRBIN) # New directory structuring

all: yes

yes: yes.o
	$(CC) -o yes yes.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

yes.o: yes.c \
	$(INC)/stdio.h

install: all
	 $(INS) -f $(INSDIR) -m 0711 -u $(OWN) -g $(GRP) yes

clean:
	rm -f yes.o

clobber: clean
	rm -f yes

lintit:
	$(LINT) $(LINTFLAGS) yes.c

