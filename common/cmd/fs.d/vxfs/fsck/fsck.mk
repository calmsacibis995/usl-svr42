#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# @(#)usr/src/common/cmd/fs.d/vxfs/fsck/fsck.mk	1.6 25 May 1992 23:36:45 - 
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/vxfs/fsck/fsck.mk	1.5"

# Copyright (c) 1991, 1992 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
# UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
# LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
# IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
# OR DISCLOSURE.
# 
# THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
# TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
# OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
# EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
# 
#               RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#               VERITAS SOFTWARE
# 4800 GREAT AMERICA PARKWAY, SUITE 420, SANTA CLARA, CA 95054

include $(CMDRULES)
LIBDIR	= ../lib
LOCALINC=-I$(LIBDIR)
INSDIR1 = $(USRLIB)/fs/vxfs
INSDIR2 = $(ETC)/fs/vxfs
OWN = bin
GRP = bin

OBJS	= \
	dir.o \
	extent.o \
	extop.o \
	inode.o \
	links.o \
	lwrite.o \
	main.o \
	replay.o \
	subr.o \
	subreplay.o \
	super.o

PROBEFILE = dir.c
MAKEFILE = fsck.mk
BINARIES = fsck fsck.dy

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -f $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

install: all
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) fsck.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) fsck.dy

clean:
	-rm -f $(OBJS)

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

binaries: $(BINARIES)

fsck: $(OBJS) $(LIBDIR)/libvxfs.a
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBDIR)/libvxfs.a $(LDLIBS) $(ROOTLIBS)

fsck.dy: $(OBJS) $(LIBDIR)/libvxfs.a
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBDIR)/libvxfs.a $(LDLIBS) $(SHLIBS)

dir.o:	dir.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_log.h \
	$(INC)/sys/fs/vx_bitmaps.h \
	$(INC)/sys/fs/vx_tran.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/malloc.h \
	fsck.h

extent.o:	extent.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_log.h \
	$(INC)/sys/fs/vx_bitmaps.h \
	$(INC)/sys/fs/vx_tran.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/ctype.h \
	$(INC)/malloc.h \
	fsck.h

extop.o:	extop.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_log.h \
	$(INC)/sys/fs/vx_bitmaps.h \
	$(INC)/sys/fs/vx_tran.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/malloc.h \
	fsck.h \
	replay.h

inode.o:	inode.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_log.h \
	$(INC)/sys/fs/vx_bitmaps.h \
	$(INC)/sys/fs/vx_tran.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	fsck.h

links.o:	links.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_log.h \
	$(INC)/sys/fs/vx_bitmaps.h \
	$(INC)/sys/fs/vx_tran.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/malloc.h \
	$(INC)/fcntl.h \
	fsck.h

lwrite.o:	lwrite.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_log.h \
	$(INC)/sys/fs/vx_bitmaps.h \
	$(INC)/sys/fs/vx_tran.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/malloc.h \
	fsck.h \
	replay.h

main.o:	main.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_log.h \
	$(INC)/sys/fs/vx_bitmaps.h \
	$(INC)/sys/fs/vx_tran.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/malloc.h \
	$(INC)/fcntl.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(LIBDIR)/fs_subr.h \
	fsck.h

replay.o:	replay.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_log.h \
	$(INC)/sys/fs/vx_bitmaps.h \
	$(INC)/sys/fs/vx_tran.h \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/malloc.h \
	fsck.h \
	replay.h

subr.o:	subr.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_log.h \
	$(INC)/sys/fs/vx_bitmaps.h \
	$(INC)/sys/fs/vx_tran.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/malloc.h \
	fsck.h

subreplay.o:	subreplay.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_log.h \
	$(INC)/sys/fs/vx_bitmaps.h \
	$(INC)/sys/fs/vx_tran.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/malloc.h \
	fsck.h \
	replay.h

super.o:	super.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/uio.h \
	$(INC)/sys/fs/vx_fs.h \
	$(INC)/sys/fs/vx_param.h \
	$(INC)/sys/fs/vx_inode.h \
	$(INC)/sys/fs/vx_dir.h \
	$(INC)/sys/fs/vx_log.h \
	$(INC)/sys/fs/vx_bitmaps.h \
	$(INC)/sys/fs/vx_tran.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/malloc.h \
	$(INC)/fcntl.h \
	fsck.h \
	$(INC)/ustat.h \
	$(INC)/sys/stat.h

