#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)nice:nice.mk	1.4.4.2"
#ident "$Header: nice.mk 1.2 91/05/23 $"

include $(CMDRULES)

#	nice make file

OWN = bin
GRP = bin

INSDIR = $(USRBIN)
SOURCE = nice.c
LDLIBS = -lm

all:	nice

nice: nice.o
	$(CC) -o nice nice.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

nice.o: $(SOURCE) \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/priv.h $(INC)/sys/privilege.h \
	$(INC)/mac.h $(INC)/sys/mac.h \
	$(INC)/sys/errno.h

install: all
	 $(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) nice

clean:
	rm -f nice.o

clobber: clean
	 rm -f nice

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCE)

