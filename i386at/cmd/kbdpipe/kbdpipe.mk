#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kbdpipe:kbdpipe.mk	1.2.1.3"
#ident "$Header: "

include $(CMDRULES)

all: kbdpipe

kbdpipe: kbdpipe.o
	$(CC) kbdpipe.o -o $@ $(LDFLAGS)

install:	all
	$(INS) -f $(USRBIN) kbdpipe

clean:
	rm -f *.o

clobber:	clean
	rm -f kbdpipe
