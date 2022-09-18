#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pseudo:pseudo.mk	1.1.3.2"
#ident  "$Header: pseudo.mk 1.3 91/06/28 $"

include $(CMDRULES)

OWN = bin
GRP = bin

all: pseudo

pseudo: pseudo.o
	$(CC) pseudo.o -o pseudo $(LDFLAGS) $(LDLIBS) $(SHLIBS)

pseudo.o: pseudo.c \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/termio.h \
	$(INC)/sys/stropts.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/errno.h \
	$(INC)/stdio.h

install: all
	 $(INS) -f $(USRBIN) -m 0755 -u $(OWN) -g $(GRP) pseudo

clean:
	rm -f pseudo.o

clobber: clean
	rm -f pseudo

lintit:
	$(LINT) $(LINTFLAGS) *.c
