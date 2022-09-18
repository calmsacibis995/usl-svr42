#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libnsl:common/lib/libnsl/rexec/rexec.mk	1.1.3.4"
#ident "$Header: rexec.mk 1.3 91/03/14 $"

include $(LIBRULES)

LOCALDEF = $(PICFLAG)
SOURCES = rxlib.c
OBJECTS = rxlib.o
LIBOBJECTS = ../rxlib.o
SRCS = $(OBJS:%.o=%.c)

all:	$(OBJECTS)
	cp $(OBJECTS) ../


rxlib.o:	rxlib.c \
		$(INC)/sys/byteorder.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/termio.h \
		$(INC)/stdio.h \
		$(INC)/stropts.h \
		$(INC)/rx.h \
		rxmsg.h

lint:
	$(LINT) -I$(INC) $(SOURCES)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(LIBOBJECTS)

size: all
	$(SIZE) $(LIBOBJECTS)

strip: all
	$(STRIP) $(LIBOBJECTS)
