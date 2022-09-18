#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)machid:machid.mk	1.1"
#ident	"$Header: $"

include $(CMDRULES)

LOCALDEF = -DAT386=1 -DSCSI

all:		machid

machid:		machid.o 
	$(CC) -o machid  machid.o   $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)


clean:
	rm -f machid.o

clobber: clean
	rm -f machid.o machid

install: all
	 $(INS) -f  $(ETC) -m 0755 -u bin -g bin machid
