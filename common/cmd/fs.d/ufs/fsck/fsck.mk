#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:ufs/fsck/fsck.mk	1.5.9.3"
#ident "$Header: fsck.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/ufs
INSDIR2 = $(ETC)/fs/ufs
OWN = bin
GRP = bin

OBJS=   dir.o inode.o pass1.o pass1b.o pass2.o \
	pass3.o pass4.o pass5.o setup.o utilities.o
UFSOBJS= ufs_subr.o ufs_tables.o

all: fsck

fsck: $(OBJS) $(UFSOBJS) main.o
	$(CC) $(LDFLAGS) -o $@ main.o $(OBJS) $(UFSOBJS) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o $@.dy main.o $(OBJS) $(UFSOBJS) $(LDLIBS) $(SHLIBS)

install: fsck
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck.dy

clean:     
	-rm -f $(OBJS) $(UFSOBJS) main.o
	
clobber: clean
	rm -f fsck fsck.dy
