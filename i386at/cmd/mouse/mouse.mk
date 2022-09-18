#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mouse:mouse.mk	1.3.3.4"

include	$(CMDRULES)

all:	mousemgr mouseadmin

install:	all 
	$(INS) -f $(USRBIN) mouseadmin
	$(INS) -f $(USRLIB) -m 0775 -u root -g sys mousemgr

clean:
	rm -f *.o

clobber: clean
	rm -f mousemgr mouseadmin

mousemgr:	mousemgr.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS)

mouseadmin:	mouseadmin.o
	$(CC) -o mouseadmin mouseadmin.o $(LDFLAGS) $(LDLIBS) -lcurses
