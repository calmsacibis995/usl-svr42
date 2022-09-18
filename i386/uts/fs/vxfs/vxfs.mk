#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# usr/src/i386/uts/fs/vxfs/vxfs.mk	1.10 18 May 1992 18:16:03 - 
#ident	"@(#)uts-x86:fs/vxfs/vxfs.mk	1.8"

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

include $(UTSRULES)

KBASE    = ../..
LOCALDEF =
FS	 = $(CONF)/pack.d/vxfs/Driver.o

CFILES	= \
	vx_alloc.c \
	vx_bio.c \
	vx_bitmaps.c \
	vx_bmap.c \
	vx_dio.c \
	vx_dira.c \
	vx_dirl.c \
	vx_dirop.c \
	vx_getpage.c \
	vx_ialloc.c \
	vx_inode.c \
	vx_ioctl.c \
	vx_itrunc.c \
	vx_log.c \
	vx_map.c \
	vx_putpage.c \
	vx_rdwri.c \
	vx_reorg.c \
	vx_ted.c \
	vx_snap.c \
	vx_tran.c \
	vx_vfsops.c \
	vx_vnops.c

#
# These C files are generated on the fly.  They exist
# so that the makefule rules are easier to handle.
#
CFILES1 = \
	vjfs_bio.c \
	vjfs_ioctl.c \
	vjfs_rdwri.c \
	vjfs_vfsops.c

#
# This macros are used by the depend target while computing
# the dependancy list for each of CFILES1
#
CFLAGS1 = $(CFLAGS)
DEFLIST1 = $(DEFLIST)

VJFS_OBJS = \
	vx_alloc.o \
	vjfs_bio.o \
	vx_bitmaps.o \
	vx_bmap.o \
	vx_dira.o \
	vx_dirl.o \
	vx_dirop.o \
	vx_getpage.o \
	vx_ialloc.o \
	vx_inode.o \
	vjfs_ioctl.o \
	vx_itrunc.o \
	vx_log.o \
	vx_map.o \
	vx_putpage.o \
	vjfs_rdwri.o \
	vx_ted.o \
	vx_tran.o \
	vjfs_vfsops.o \
	vx_vnops.o

VXFS_OBJS = \
	vx_alloc.o \
	vx_bio.o \
	vx_bitmaps.o \
	vx_bmap.o \
	vx_dio.o \
	vx_dira.o \
	vx_dirl.o \
	vx_dirop.o \
	vx_getpage.o \
	vx_ialloc.o \
	vx_inode.o \
	vx_ioctl.o \
	vx_itrunc.o \
	vx_log.o \
	vx_map.o \
	vx_putpage.o \
	vx_rdwri.o \
	vx_reorg.o \
	vx_snap.o \
	vx_ted.o \
	vx_tran.o \
	vx_vfsops.o \
	vx_vnops.o

PROBEFILE = vx_alloc.c
MAKEFILE = vxfs.mk
BINARIES = Driver_vj.o

all:	ID
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi
	cp $(BINARIES) $(FS)

clean:
	-rm -f $(VXFS_OBJS) $(VJFS_OBJS) $(CFILES1)

clobber: clean
	-$(IDINSTALL) -e -R$(CONF) -d vxfs
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" Driver_vx.o ;\
		rm -f $(BINARIES) Driver_vx.o ;\
	fi

#
# Configuration Section
#
ID:
	cd vxfs.cf; $(IDINSTALL) -R$(CONF) -M vxfs

headinstall: \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_tran.h
	[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	[ -d t ] || mkdir t
	for fl in vx_bitmaps.h vx_buf.h vx_dir.h vx_ext.h vx_fs.h vx_inode.h \
		    vx_ioctl.h vx_log.h vx_param.h vx_proto.h vx_tran.h; do \
		rm -f t/$$fl ;\
		grep -v TED_ $(KBASE)/fs/vxfs/$$fl > t/$$fl ;\
		$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) t/$$fl ;\
	done
	rm -fr t
#	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/vxfs/vx_tran.h

binaries: $(BINARIES) Driver_vx.o

Driver_vj.o: $(VJFS_OBJS)
	$(LD) -r -o $@ $(VJFS_OBJS)

Driver_vx.o: $(VXFS_OBJS)
	$(LD) -r -o $@ $(VXFS_OBJS)

vjfs_bio.c: vx_bio.c
	cp vx_bio.c vjfs_bio.c

vjfs_ioctl.c: vx_ioctl.c
	cp vx_ioctl.c vjfs_ioctl.c

vjfs_rdwri.c: vx_rdwri.c
	cp vx_rdwri.c vjfs_rdwri.c

vjfs_vfsops.c: vx_vfsops.c
	cp vx_vfsops.c vjfs_vfsops.c

vjfs_bio.o:
	$(CC) $(CFLAGS) -D VXFS_VJFS $(DEFLIST) -c $<

vjfs_ioctl.o:
	$(CC) $(CFLAGS) -D VXFS_VJFS $(DEFLIST) -c $<

vjfs_rdwri.o:
	$(CC) $(CFLAGS) -D VXFS_VJFS $(DEFLIST) -c $<

vjfs_vfsops.o:
	$(CC) $(CFLAGS) -D VXFS_VJFS $(DEFLIST) -c $<

depend:

#
#	Header dependencies
#

vx_alloc.o: vx_alloc.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h

vx_bio.o: vx_bio.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/page.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/mem/vmparam.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/inline.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/var.h \
	$(KBASE)/util/weitek.h

vx_bitmaps.o: vx_bitmaps.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h

vx_bmap.o: vx_bmap.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vx_dio.o: vx_dio.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/as.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/pvn.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/seg_map.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vx_dira.o: vx_dira.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dirent.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/mode.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/stat.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h

vx_dirl.o: vx_dirl.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dirent.h \
	$(KBASE)/fs/dnlc.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/mode.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/stat.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vx_dirop.o: vx_dirop.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/acc/priv/privilege.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dirent.h \
	$(KBASE)/fs/dnlc.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/mode.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/stat.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h

vx_getpage.o: vx_getpage.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dirent.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/flock.h \
	$(KBASE)/fs/fs_subr.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/as.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/hat.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/page.h \
	$(KBASE)/mem/pvn.h \
	$(KBASE)/mem/rm.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/seg_map.h \
	$(KBASE)/mem/seg_vn.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/mem/vmparam.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/mman.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vx_ialloc.o: vx_ialloc.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/flock.h \
	$(KBASE)/fs/mode.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/stat.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vx_inode.o: vx_inode.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/acc/priv/privilege.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dnlc.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/fs_subr.h \
	$(KBASE)/fs/fsinode.h \
	$(KBASE)/fs/mode.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/stat.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/pvn.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h

vx_ioctl.o: vx_ioctl.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/acc/priv/privilege.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vx_itrunc.o: vx_itrunc.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/flock.h \
	$(KBASE)/fs/fs_subr.h \
	$(KBASE)/fs/mode.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/stat.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/page.h \
	$(KBASE)/mem/pvn.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/mem/vmparam.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vx_log.o: vx_log.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/mode.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/stat.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h

vx_map.o: vx_map.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/as.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/seg_vn.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/mem/vmsystm.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/mman.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vx_putpage.o: vx_putpage.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dirent.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/flock.h \
	$(KBASE)/fs/fs_subr.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/as.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/hat.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/page.h \
	$(KBASE)/mem/pvn.h \
	$(KBASE)/mem/rm.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/seg_map.h \
	$(KBASE)/mem/seg_vn.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/mem/vmparam.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/mman.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vx_rdwri.o: vx_rdwri.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/acc/priv/privilege.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dirent.h \
	$(KBASE)/fs/dnlc.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/flock.h \
	$(KBASE)/fs/fs_subr.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/as.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/hat.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/page.h \
	$(KBASE)/mem/pvn.h \
	$(KBASE)/mem/rm.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/seg_kmem.h \
	$(KBASE)/mem/seg_map.h \
	$(KBASE)/mem/seg_vn.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/mem/vmparam.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/mman.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vx_reorg.o: vx_reorg.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/acc/priv/privilege.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vx_ted.o: vx_ted.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dnlc.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/mem/vmsystm.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/mman.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/var.h \
	$(KBASE)/util/weitek.h

vx_snap.o: vx_snap.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/acc/priv/privilege.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dnlc.h \
	$(KBASE)/fs/fbuf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/fs_subr.h \
	$(KBASE)/fs/mount.h \
	$(KBASE)/fs/pathname.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/specfs/snode.h \
	$(KBASE)/fs/statvfs.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/page.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/seg_map.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/mem/vmparam.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/var.h \
	$(KBASE)/util/weitek.h

vx_tran.o: vx_tran.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/mode.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/stat.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h

vx_vfsops.o: vx_vfsops.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/acc/priv/privilege.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dnlc.h \
	$(KBASE)/fs/fbuf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/fs_subr.h \
	$(KBASE)/fs/mount.h \
	$(KBASE)/fs/pathname.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/specfs/snode.h \
	$(KBASE)/fs/statvfs.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mod/moddefs.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/var.h \
	$(KBASE)/util/weitek.h

vx_vnops.o: vx_vnops.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/acc/priv/privilege.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dirent.h \
	$(KBASE)/fs/dnlc.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/flock.h \
	$(KBASE)/fs/fs_subr.h \
	$(KBASE)/fs/pathname.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/specfs/snode.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/as.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/hat.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/page.h \
	$(KBASE)/mem/pvn.h \
	$(KBASE)/mem/rm.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/seg_map.h \
	$(KBASE)/mem/seg_vn.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/mem/vmmeter.h \
	$(KBASE)/mem/vmparam.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/mman.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vjfs_bio.o: vjfs_bio.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/page.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/mem/vmparam.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/inline.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/var.h \
	$(KBASE)/util/weitek.h

vjfs_ioctl.o: vjfs_ioctl.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/acc/priv/privilege.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vjfs_rdwri.o: vjfs_rdwri.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/acc/priv/privilege.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dirent.h \
	$(KBASE)/fs/dnlc.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/flock.h \
	$(KBASE)/fs/fs_subr.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/as.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/hat.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/page.h \
	$(KBASE)/mem/pvn.h \
	$(KBASE)/mem/rm.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/seg_kmem.h \
	$(KBASE)/mem/seg_map.h \
	$(KBASE)/mem/seg_vn.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/mem/vm_hat.h \
	$(KBASE)/mem/vmparam.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/mman.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysinfo.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/weitek.h

vjfs_vfsops.o: vjfs_vfsops.c \
	$(KBASE)/acc/mac/mac.h \
	$(KBASE)/acc/priv/privilege.h \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dnlc.h \
	$(KBASE)/fs/fbuf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/fs_subr.h \
	$(KBASE)/fs/mount.h \
	$(KBASE)/fs/pathname.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/specfs/snode.h \
	$(KBASE)/fs/statvfs.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vnode.h \
	$(KBASE)/fs/vxfs/vx_bitmaps.h \
	$(KBASE)/fs/vxfs/vx_buf.h \
	$(KBASE)/fs/vxfs/vx_dir.h \
	$(KBASE)/fs/vxfs/vx_ext.h \
	$(KBASE)/fs/vxfs/vx_fs.h \
	$(KBASE)/fs/vxfs/vx_inode.h \
	$(KBASE)/fs/vxfs/vx_ioctl.h \
	$(KBASE)/fs/vxfs/vx_log.h \
	$(KBASE)/fs/vxfs/vx_param.h \
	$(KBASE)/fs/vxfs/vx_proto.h \
	$(KBASE)/fs/vxfs/vx_ted.h \
	$(KBASE)/fs/vxfs/vx_tran.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/faultcatch.h \
	$(KBASE)/mem/faultcode.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/mem/seg.h \
	$(KBASE)/mem/swap.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp.h \
	$(KBASE)/proc/priocntl.h \
	$(KBASE)/proc/proc.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/proc/siginfo.h \
	$(KBASE)/proc/signal.h \
	$(KBASE)/proc/tss.h \
	$(KBASE)/proc/user.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/resource.h \
	$(KBASE)/svc/systm.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/mod/moddefs.h \
	$(KBASE)/util/mp.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/spl.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/types.h \
	$(KBASE)/util/var.h \
	$(KBASE)/util/weitek.h
