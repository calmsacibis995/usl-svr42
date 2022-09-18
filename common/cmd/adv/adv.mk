#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)adv:adv.mk	1.4.13.2"
#ident  "$Header: adv.mk 1.5 91/07/26 $"

include $(CMDRULES)

LDLIBS=-lns
OWN=bin
GRP=bin
FRC =

LINTFLAGS = -ux #-unx

all: adv

adv: adv.o 
	$(CC) -o adv adv.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

adv.o: adv.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/nserve.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/stdlib.h

install: all
	-rm -f /usr/bin/adv
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) adv
	-$(SYMLINK) /usr/sbin/adv $(USRBIN)/adv

clean:
	rm -f adv.o

clobber: clean
	rm -f adv

lintit:
	$(LINT) $(LINTFLAGS) adv.c

FRC:
