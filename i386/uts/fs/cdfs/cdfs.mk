#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Copyright (c) 1991, 1992  Intel Corporation
#	All Rights Reserved

#	INTEL CORPORATION CONFIDENTIAL INFORMATION

#	This software is supplied to USL under the terms of a license
#	agreement with Intel Corporation and may not be copied nor
#	disclosed except in accordance with the terms of that agreement.

#ident	"@(#)uts-x86:fs/cdfs/cdfs.mk	1.14"
#ident	"$Header: $"

include $(UTSRULES)

KBASE   = ../..
INSPERM = -m 644 -u $(OWN) -g $(GRP)
FS	= $(CONF)/pack.d/cdfs/Driver.o
DRIVER = Driver.o 
FILES = \
	cdfs_vfsops.o \
	cdfs_vnops.o \
	cdfs_ioctl.o \
	cdfs_inode.o \
	cdfs_dir.o \
	cdfs_subr.o \
	cdfs_susp.o

CFILES = $(FILES:.o=.c)


all:	ID $(FS)

$(FS):	$(DRIVER)
	$(CP) $(DRIVER) $@

$(DRIVER): $(FILES)
	$(LD) -r -o $@ $(FILES)

lintit: $(FILES:.o=.ln)
	$(LINT) -s $(DEFLIST) $(FILES:.o=.ln)

$(FILES:.o=.ln): $(FILES)
	$(LINT) -s -c $(DEFLIST) $(FILES:.o=.c)

#
# Configuration Section
#
ID:
	cd cdfs.cf; $(IDINSTALL) -R$(CONF) -M cdfs 

headinstall: \
	$(KBASE)/fs/cdfs/cdfs_fs.h \
	$(KBASE)/fs/cdfs/cdfs_inode.h \
	$(KBASE)/fs/cdfs/cdfs_ioctl.h \
	$(KBASE)/fs/cdfs/cdfs_susp.h \
	$(KBASE)/fs/cdfs/cdrom.h \
	$(KBASE)/fs/cdfs/iso9660.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	[ -d $(INC)/sys/fs ] || mkdir $(INC)/sys/fs
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/cdfs/cdfs_fs.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/cdfs/cdfs_inode.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/cdfs/cdfs_ioctl.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/cdfs/cdfs_susp.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/cdfs/iso9660.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/fs/cdfs/cdrom.h

clean:
	-rm -f $(FILES) $(FILES:.o=.ln) $(DRIVER)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d cdfs

FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

