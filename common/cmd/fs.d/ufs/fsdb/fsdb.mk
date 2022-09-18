#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:ufs/fsdb/fsdb.mk	1.3.5.3"
#ident "$Header: fsdb.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/ufs
INSDIR2 = $(ETC)/fs/ufs
OWN = bin
GRP = bin

all:  fsdb

fsdb: fsdb.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(NOSHLIBS) $(LDLIBS)

install: fsdb
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsdb
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsdb

clean:
	-rm -f fsdb.o

clobber: clean
	rm -f fsdb
