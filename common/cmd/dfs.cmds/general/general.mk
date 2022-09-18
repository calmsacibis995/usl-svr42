#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dfs.cmds:general/general.mk	1.2.5.2"
#ident "$Header: general.mk 1.4 91/04/08 $"

include $(CMDRULES)

INSDIR = $(USRSBIN)
FRC =

all: general

general: general.c 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ general.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	$(INS) -f $(INSDIR) -m 0555 general

clean:
	rm -f general.o

clobber: clean
	rm -f general
FRC:
