#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/tcpip/tcpip.mk	1.2.7.3"
#ident "$Header: tcpip.mk 1.3 91/03/15 $"

#	Makefile for tcpip.so

include $(LIBRULES)

LIBNAME=	tcpip.so
OBJECTS=	tcpip.o file_db.o

LOCALDEF=-D_NSL_RPC_ABI -DPIC $(PICFLAG)
LOCALLDFLAGS=-s -dy -G -ztext -h /usr/lib/$(LIBNAME)


all:		$(LIBNAME)

tcpip.so:	$(OBJECTS)
		$(CC) $(LOCALLDFLAGS) -o $(LIBNAME) $(OBJECTS)


tcpip.o:   	$(INC)/stdio.h $(INC)/ctype.h $(INC)/sys/types.h \
	   		$(INC)/sys/socket.h $(INC)/netinet/in.h \
			$(INC)/netdb.h $(INC)/tiuser.h $(INC)/netconfig.h \
			$(INC)/netdir.h $(INC)/string.h $(INC)/sys/param.h \
			$(INC)/sys/utsname.h tcpip.c

file_db.o:	$(INC)/stdio.h $(INC)/ctype.h $(INC)/string.h  \
	   		$(INC)/sys/types.h $(INC)/sys/socket.h \
			$(INC)/netdb.h $(INC)/netinet/in.h file_db.c

clean:
		rm -f $(OBJECTS)

clobber:
		rm -f $(OBJECTS) $(LIBNAME)

install:	all
		$(INS) -f $(USRLIB) $(LIBNAME)

size:		all
		$(SIZE) $(LIBNAME)

strip:		all
		$(STRIP) $(LIBNAME)

lintit:
