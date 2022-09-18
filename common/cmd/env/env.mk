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

#ident	"@(#)env:env.mk	1.4.3.1"
#ident "$Header: env.mk 1.2 91/04/08 $"
#	env make file

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LDLIBS= -lm
SOURCE = env.c

all:	env

env:	$(SOURCE)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ env.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: env
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) env

clean:
	rm -f env.o

clobber:	clean
	  rm -f env
