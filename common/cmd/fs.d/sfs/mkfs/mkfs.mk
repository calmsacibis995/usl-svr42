#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sfs.cmds:sfs/mkfs/mkfs.mk	1.2.3.3"
#ident "$Header: mkfs.mk 1.2 91/04/11 $"

include $(CMDRULES)
INSDIR1 = $(USRLIB)/fs/sfs
INSDIR2 = $(ETC)/fs/sfs
OWN = bin
GRP = bin

all:  mkfs

mkfs: mkfs.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o $@.dy $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: mkfs
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mkfs
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mkfs
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mkfs.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mkfs.dy

clean:
	-rm -f mkfs.o

clobber: clean
	rm -f mkfs mkfs.dy
