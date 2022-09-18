#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)nfs.cmds:nfs/biod/biod.mk	1.5.4.2"
#ident	"$Header: $"

include $(CMDRULES)

INCSYS = $(INC)
LOCALDEF = -Dnetselstrings
INSDIR = $(USRLIB)/nfs
OWN = bin
GRP = bin

all: biod

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) biod

clean:
	-rm -f biod.o

clobber: clean
	-rm -f biod

biod:	biod.o
	$(CC) -o $@ $(LDFLAGS) $@.o $(LDLIBS) $(SHLIBS)

biod.o:	biod.c $(INC)/stdio.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/file.h \
	$(INCSYS)/sys/ioctl.h
