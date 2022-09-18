#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)checkeq:checkeq.mk	1.1.4.2"
#ident	"$Header: checkeq.mk 1.1 91/05/07 $"

# Makefile for checkeq

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

all: checkeq

checkeq:	checkeq.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

checkeq.o:

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) checkeq

clean:
	rm -f checkeq.o

clobber: clean
	rm -f checkeq
