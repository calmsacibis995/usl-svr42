#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libnsl:common/lib/libnsl/cr1/cr1.mk	1.1.4.4"
#ident "$Header: cr1.mk 1.4 91/06/27 $"

include $(LIBRULES)

# 
# Network Services Library: Challange/Response Scheme #1 routines
#
LOCALDEF=-DNO_IMPORT $(PICFLAG)

INCLUDES=	\
		cr1.h \
		$(INC)/cr1.h \
		$(INC)/crypt.h \
		$(INC)/fcntl.h \
		$(INC)/pwd.h \
		$(INC)/stdio.h \
		$(INC)/stdlib.h \
		$(INC)/string.h \
		$(INC)/unistd.h \
		$(INC)/rpc/types.h \
		$(INC)/rpc/xdr.h \
		$(INC)/sys/types.h

LIBOBJS = getkey.o

OBJS =	../getkey.o

all:       $(INCLUDES) $(LIBOBJS)

install:

lintit:
	$(LINT) getkey.c
clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(OBJS)

.c.o:
	$(CC) $(CFLAGS) $(DEFLIST) -c $*.c && cp $(*F).o ..
