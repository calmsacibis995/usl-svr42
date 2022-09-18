#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)strings:common/cmd/strings/strings.mk	1.5.5.3"
#ident "$Header: strings.mk 1.2 91/06/03 $"

include $(CMDRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved
#	This Module contains Proprietary Information of Microsoft
#	Corporation and should be treated as Confidential.
# Makefile for strings

OWN = bin
GRP = bin

LDLIBS = $(LIBELF)

all: strings

strings: strings.o
	$(CC) -o strings strings.o $(LDFLAGS) $(LDLIBS)

strings.o: strings.c \
	$(INC)/stdio.h \
	x.out.h \
	$(INC)/ctype.h \
	$(INC)/libelf.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) strings

clean:
	rm -f strings.o

clobber: clean
	rm -f strings

lintit:
	$(LINT) $(LINTFLAGS) strings.c

