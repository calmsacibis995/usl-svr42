#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)messages:uxnsu/msgs.mk	1.1.1.2"
#ident  "$Header: msgs.mk 1.2 91/07/02 $"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs nsu.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 nsu.str

lintit : 

clean :
	rm -f nsu.str

clobber : clean

