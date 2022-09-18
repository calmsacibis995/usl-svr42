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

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fdetach:fdetach.mk	1.1.5.1"
#ident "$Header: fdetach.mk 1.2 91/04/09 $"

#	fdetach make file

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

all:	fdetach

fdetach: fdetach.o
	$(CC) -o $@ fdetach.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

fdetach.o: fdetach.c \
	$(INC)/stdio.h

install: fdetach
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) fdetach

clean:
	-rm -f fdetach.o

clobber: clean
	rm -f fdetach
