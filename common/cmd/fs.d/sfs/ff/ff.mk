#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sfs.cmds:sfs/ff/ff.mk	1.1.3.2"
#ident "$Header: ff.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/sfs
OWN = bin
GRP = bin

OBJS=

all:	ff

ff:	ff.o
	$(CC) $(LDFLAGS) -o $@ $@.o $(ROOTLIBS) $(LDLIBS)

ff.o: 	ff.c  \
	$(INC)/pwd.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/stat.h 

install: ff
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	$(INS) -f $(INSDIR1) -u $(OWN) -g $(GRP) ff

clean:
	-rm -f ff.o

clobber: clean
	rm -f ff
