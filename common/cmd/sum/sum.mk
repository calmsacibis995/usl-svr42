#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sum:sum.mk	1.4.6.2"
#ident "$Header: sum.mk 1.3 91/03/19 $"

include $(CMDRULES)

#	Makefile for sum

OWN = bin
GRP = bin

all: sum

sum: sum.c \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) sum

clean:
	rm -f sum.o

clobber: clean
	rm -f sum

lintit:
	$(LINT) $(LINTFLAGS) sum.c

