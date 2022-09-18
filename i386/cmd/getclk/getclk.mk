#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)getclk:getclk.mk	1.1.3.3"
#ident  "$Header: getclk.mk 1.1 91/05/17 $"

include $(CMDRULES)

OWN = root
GRP = sys

all : getclk

getclk: getclk.o 
	$(CC) -o getclk getclk.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

getclk.o: getclk.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/rtc.h

install: all
	$(INS) -f $(USRSBIN) -m 0744 -u $(OWN) -g $(GRP) getclk

clean:
	rm -f getclk.o

clobber: clean
	rm -f getclk

lintit:
	$(LINT) $(LINTFLAGS) getclk.c
