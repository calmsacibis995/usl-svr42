#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dname:dname.mk	1.4.10.2"
#ident "$Header: dname.mk 1.4 91/06/05 $"

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin
LDLIBS = -lns
FRC =

all: dname

dname: dname.c 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ dname.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	-rm -f $(USRBIN)/dname
	$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) dname
	-$(SYMLINK) /usr/sbin/dname $(USRBIN)/dname

clean:
	rm -f *.o

clobber: clean
	rm -f dname
FRC:
