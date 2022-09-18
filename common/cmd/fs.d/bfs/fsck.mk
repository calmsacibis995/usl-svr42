#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bfs.cmds:bfs/fsck.mk	1.11.8.4"
#ident "$Header: fsck.mk 1.2 91/04/11 $"

include $(CMDRULES)

OWN = bin
GRP = bin

FRC =

FILES =\
	fsck.o

all: fsck

fsck: $(FILES)
	$(CC) $(LDFLAGS) -o $@ $(FILES) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o $@.dy $(FILES) $(LDLIBS) $(SHLIBS)

clean:
	rm -f *.o

install: fsck
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) fsck.dy
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) fsck.dy


clobber: clean
	rm -f fsck fsck.dy

#
# Header dependencies
#

fsck.o: fsck.c \
	$(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/fs/bfs.h \
	$(INC)/sys/fs/bfs_compact.h \
	$(INC)/sys/stat.h \
	$(FRC)
