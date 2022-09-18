#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfs.cmds:rfs/dfshares/dfshares.mk	1.2.5.3"
#ident	"$Header: dfshares.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/rfs
OWN = root
GRP = bin

LDLIBS = -lns
FRC =

all: dfshares

dfshares: dfshares.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 04555 -u $(OWN) -g $(GRP) dfshares

clean:
	rm -f dfshares.o

clobber: clean
	rm -f dfshares
FRC:
