#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/sfs/sfs.mk	1.7"
#ident "$Header: sfs.mk 1.2 91/03/25 $"

include $(UTSRULES)

KBASE    = ../..
LOCALDEF = -DQUOTA
FS	 = $(CONF)/pack.d/sfs/Driver.o
FILES = \
	sfs_alloc.o \
	sfs_bmap.o \
	sfs_blklst.o \
	sfs_dir.o \
	sfs_inode.o \
	sfs_qcalls.o \
	sfs_qlims.o \
	sfs_quota.o \
	sfs_subr.o \
	sfs_tables.o \
	sfs_vfsops.o \
	sfs_vnops.o

CFILES = $(FILES:.o=.c)


all:	ID $(FS)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#
ID:
	cd sfs.cf; $(IDINSTALL) -R$(CONF) -M sfs


clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d sfs


headinstall: \
	$(KBASE)/fs/sfs/sfs_fs.h \
	$(KBASE)/fs/sfs/sfs_fsdir.h \
	$(KBASE)/fs/sfs/sfs_inode.h \
	$(KBASE)/fs/sfs/sfs_quota.h \
	$(KBASE)/fs/sfs/sfs_tables.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	[ -d $(INC)/sys/fs ] || mkdir $(INC)/sys/fs
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/sfs/sfs_fs.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/sfs/sfs_fsdir.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/sfs/sfs_inode.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/sfs/sfs_quota.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/sfs/sfs_tables.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/sfs/sfs_tables.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

