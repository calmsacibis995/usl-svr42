#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dfs.cmds:share/share.mk	1.3.5.2"
#ident "$Header: share.mk 1.3 91/04/08 $"

include $(CMDRULES)

INSDIR = $(USRSBIN)
FRC =

all: share

share: share.c 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ share.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	$(INS) -f $(INSDIR) -m 0555 share

clean:
	rm -f share.o

clobber: clean
	rm -f share
FRC:
