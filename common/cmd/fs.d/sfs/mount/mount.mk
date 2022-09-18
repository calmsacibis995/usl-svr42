#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sfs.cmds:sfs/mount/mount.mk	1.4.5.3"
#ident "$Header: mount.mk 1.2 91/04/11 $"

include $(CMDRULES)
INSDIR1 = $(USRLIB)/fs/sfs
INSDIR2 = $(ETC)/fs/sfs
OWN = bin
GRP = bin

all:  mount

mount: mount.o
	$(CC) $(LDFLAGS) -o $@ $@.o $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o $@.dy $@.o $(LDLIBS) $(SHLIBS)

mount.o: mount.c \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/mac.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mntent.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vfs.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/vfstab.h \
	$(INC)/sys/fsid.h

install: mount
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount.dy

clean:
	-rm -f mount.o

clobber: clean
	rm -f mount mount.dy
