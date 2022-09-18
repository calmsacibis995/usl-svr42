#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sfs.cmds:sfs/labelit/labelit.mk	1.2.3.2"
#ident "$Header: labelit.mk 1.3 91/04/29 $"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/sfs
INSDIR2 = $(ETC)/fs/sfs
OWN = bin
GRP = bin

all:  labelit

labelit: labelit.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

labelit.o: labelit.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/fs/sfs_fs.h

install: labelit
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) labelit
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) labelit
clean:
	-rm -f labelit.o

clobber: clean
	rm -f labelit
