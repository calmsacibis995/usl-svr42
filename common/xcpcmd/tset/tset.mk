#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcptset:tset.mk	1.2.2.4"
#ident  "$Header: tset.mk 1.2 91/07/11 $"

include $(CMDRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved
#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.
#
#	@(#) tset.mk 1.1 88/03/29 tset:tset.mk
#

OWN = bin
GRP = bin

LOCALDEF = $(GCFLAGS)
LDLIBS = $(GLDLIBS) $(SHLIBS) -lcurses -lcmd -lgen

REGCMP = regcmp

SRCS = tset.c
OBJS = tset.o
EXES = tset

all: tset

tset: $(OBJS)
	${CC} -o tset tset.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

tset.o: tset.c \
	$(INC)/ctype.h \
	$(INC)/memory.h \
	$(INC)/string.h \
	delays.i \
	$(INC)/sys/types.h \
	$(INC)/curses.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/sgtty.h \
	$(INC)/stdio.h

delays.i: delays
	$(REGCMP) delays

install: all
	$(INS) -f $(USRBIN) -m 0755 -u $(OWN) -g $(GRP) tset 

cmp: all
	cmp tset $(USRBIN)/tset

clean:
	rm -f $(OBJS) delays.i

clobber: clean
	rm -f $(EXES)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)
