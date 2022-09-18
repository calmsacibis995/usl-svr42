#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)xcpmessage:message.mk	1.1.2.2"
#ident  "$Header: message.mk 1.2 91/07/11 $"

include $(CMDRULES)


OWN = 
GRP = 

all: message

message: message.o
	$(CC) -o message message.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

message.o: message.c \
	$(INC)/stdio.h \
	$(INC)/termio.h \
	$(INC)/signal.h \
	$(INC)/setjmp.h

install: all
	$(INS) -f $(USRBIN) message

clean:
	rm -f message.o 

clobber: clean
	rm -f message

lintit:
	$(LINT) $(LINTFLAGS) message.c
