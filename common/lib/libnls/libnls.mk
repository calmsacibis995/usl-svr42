#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnls:common/lib/libnls/libnls.mk	1.2.7.1"
#ident	"$Header: $"

#
# libnls.mk: makefile for network listener library
#

include $(LIBRULES)

# if debug is needed then add -DDEBUGMODE to following line
LIBNLS = libnls.a
LINTFLAGS = -b -x

OWN	= bin
GRP	= bin

SRC = nlsenv.c nlsdata.c nlsrequest.c

OBJ = nlsenv.o nlsdata.o nlsrequest.o

all:	libnls


lintit:
		$(LINT) $(LINTFLAGS) $(SRC)

libnls:	$(LIBNLS)

$(LIBNLS):	$(OBJ)
	$(AR) $(ARFLAGS) $(LIBNLS) $(OBJ)

.PRECIOUS:	$(LIBNLS)

# listener library routines and /usr/include headers:

nlsenv.o:	$(INC)/ctype.h $(INC)/listen.h $(INC)/sys/tiuser.h
nlsdata.o:	$(INC)/sys/tiuser.h
nlsrequest.o:	$(INC)/stdio.h $(INC)/ctype.h $(INC)/fcntl.h \
				$(INC)/errno.h $(INC)/string.h $(INC)/sys/tiuser.h \
				$(INC)/listen.h

install:	all
		$(INS) -f $(USRLIB) -u $(OWN) -g $(GRP) -m 644 $(LIBNLS)

clean:
	-rm -f *.o

clobber: clean
	-rm -f libnls.a

FRC:
