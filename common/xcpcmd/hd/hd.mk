#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcphd:hd.mk	1.2.2.2"
#ident  "$Header: hd.mk 1.2 91/07/11 $"

include $(CMDRULES)

# Makefile for hd
# to install when not privileged
# set $(CH) in the environment to #

OWN = bin
GRP = bin

all: hd

hd: hd.o
	$(CC) -o hd hd.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

hd.o: hd.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/errno.h $(INC)/sys/errno.h

install: all
	rm -f $(USRBIN)/hd
	$(INS) -f $(USRBIN) -m 0775 -u $(OWN) -g $(GRP) hd

clean:
	rm -f hd.o

clobber: clean
	rm -f hd

lintit:
	$(LINT) $(LINTFLAGS) hd.c
