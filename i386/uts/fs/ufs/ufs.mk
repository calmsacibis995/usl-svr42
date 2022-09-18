#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/ufs/ufs.mk	1.7"
#ident "$Header: ufs.mk 1.1 91/03/21 $"

include $(UTSRULES)

KBASE    = ../..
LOCALDEF = -DQUOTA
FS	 = $(CONF)/pack.d/ufs/Driver.o
FILES = \
	ufs_vfsops.o 

CFILES = $(FILES:.o=.c)


all:	ID $(FS)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#
ID:
	cd ufs.cf; $(IDINSTALL) -R$(CONF) -M ufs


clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d ufs

headinstall: \
	$(KBASE)/fs/ufs/ufs_fs.h \
	$(KBASE)/fs/ufs/ufs_fsdir.h \
	$(KBASE)/fs/ufs/ufs_inode.h \
	$(KBASE)/fs/ufs/ufs_quota.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	[ -d $(INC)/sys/fs ] || mkdir $(INC)/sys/fs
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/ufs/ufs_fs.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/ufs/ufs_fsdir.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/ufs/ufs_inode.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/ufs/ufs_quota.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

