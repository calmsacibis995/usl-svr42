#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rmntstat:rmntstat.mk	1.3.14.2"
#ident  "$Header: rmntstat.mk 1.3 91/06/28 $"

include $(CMDRULES)


OWN = bin
GRP = bin

LINTFLAGS = -ux #-unx

.SUFFIXES: .o .ln

.c.ln :
	$(LINT) $(LINTFLAGS) -c $*.c

all: rmntstat

rmntstat:  rmntstat.o
	$(CC) rmntstat.o -o rmntstat $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rmntstat.o: rmntstat.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h

install: all
	-rm -f $(USRBIN)/rmntstat
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) rmntstat
	-$(SYMLINK) /usr/sbin/rmntstat $(USRBIN)/rmntstat

clean:
	rm -f rmntstat.o

clobber: clean
	rm -f rmntstat

lintit:  rmntstat.ln
	$(LINT) $(LINTFLAGS) rmntstat.ln
