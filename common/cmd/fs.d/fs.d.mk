#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)fs.cmds:common/cmd/fs.d/fs.d.mk	1.31.16.8"
#ident "$Header: fs.d.mk 1.5 91/06/28 $"
#  /usr/src/cmd/fs.d is the directory of all generic commands
#  whose executable reside in $(INSDIR).
#  Fstype specific commands are in subdirectories under fs.d
#  named by fstype (ex: the generic mount is in this directory and
#  built by this makefile, but the s5 specific mount in in ./s5/mount.c,
#  built by ./s5/s5.mk)

include $(CMDRULES)
INSDIR = $(SBIN)
INSDIR2= $(USRSBIN)
BINDIR = $(USRBIN)
OLDBINDIR = $(ROOT)/$(MACH)/bin
OLDDIR = $(ETC)
LDLIBS = -ladm

.MUTEX: all install

all: all_fstyp df umount umount.dy mount mount.dy switch fsck volcopy \
	ncheck ff mklost+found

clean: clean_fstyp
	rm -f *.o

clobber: clobber_fstyp clobber_mount clobber_switch clobber_fsck \
         clobber_volcopy clobber_ncheck clobber_umount clobber_df clobber_ff \
	 clobber_mklost+found
	rm -f *.o

install: install_fstyp install_mount install_umount  install_df \
	install_switch install_fsck install_volcopy install_ncheck install_ff \
	install_mklost+found

#
#  This is to build all the fstype specific commands
#

all_fstyp:
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) all ; \
		cd .. ; \
	    fi;\
	done

install_fstyp:
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/$$i.mk ]; \
		then \
		cd $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) install ; \
		cd .. ; \
		fi;\
	done

clean_fstyp:
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/$$i.mk ]; \
		then \
			cd $$i;\
			$(MAKE) -f $$i.mk $(MAKEARGS) clean; \
			cd .. ; \
		fi;\
	done

clobber_fstyp:
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/$$i.mk ]; \
		then \
			cd $$i;\
			$(MAKE) -f $$i.mk $(MAKEARGS) clobber; \
			cd .. ; \
		fi;\
	done

#
# This is for the generic mount command
#
mount:	mount.o
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o $@ $@.o $(ROOTLIBS) $(LDLIBS)

mount.dy:	mount.o
		$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o mount.dy mount.o -dy $(LDLIBS)

mount.o:	mount.c\
	$(INC)/stdio.h\
	$(INC)/limits.h\
	$(INC)/sys/types.h\
	$(INC)/sys/stat.h\
	$(INC)/sys/statvfs.h\
	$(INC)/sys/errno.h\
	$(INC)/sys/mnttab.h\
	$(INC)/sys/vfstab.h\
	$(INC)/mac.h

clobber_mount:
	rm -f mount mount.dy

install_mount: mount mount.dy
	-rm -f $(INSDIR)/mount
	-rm -f $(INSDIR2)/mount
	-rm -f $(OLDDIR)/mount
	$(INS) -f $(INSDIR) mount
	$(INS) -f $(INSDIR2) mount
	$(INS) -f $(OLDDIR) mount
	-rm -f $(INSDIR)/mount.dy
	-rm -f $(INSDIR2)/mount.dy
	-rm -f $(OLDDIR)/mount.dy
	$(INS) -f $(INSDIR) mount.dy
	$(INS) -f $(INSDIR2) mount.dy
	$(INS) -f $(OLDDIR) mount.dy


#
# This is for the generic umount command
#
umount:	umount.o
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o  $@ $@.o $(ROOTLIBS) $(LDLIBS)

umount.dy:	umount.o
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o  $@ umount.o -dy $(LDLIBS)

umount.o:	umount.c\
	$(INC)/stdio.h \
	$(INC)/limits.h \
	$(INC)/signal.h \
	$(INC)/unistd.h \
	$(INC)/mac.h \
	$(INC)/priv.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/param.h

clobber_umount:
	rm -f umount umount.dy

install_umount: umount umount.dy
	-rm -f  $(INSDIR)/umount 
	-rm -f  $(OLDDIR)/umount 
	-rm -f $(INSDIR2)/umount
	$(INS) -f $(INSDIR) umount
	$(INS) -f $(INSDIR2) umount
	$(INS) -f $(OLDDIR) umount
	-rm -f  $(INSDIR)/umount.dy 
	-rm -f  $(OLDDIR)/umount.dy 
	-rm -f $(INSDIR2)/umount.dy
	$(INS) -f $(INSDIR) umount.dy
	$(INS) -f $(INSDIR2) umount.dy
	$(INS) -f $(OLDDIR) umount.dy

#
# This is for the generic df command
#

df:	df.c\
	$(INC)/stdio.h\
	$(INC)/priv.h\
	$(INC)/limits.h\
	$(INC)/fcntl.h\
	$(INC)/dirent.h \
	$(INC)/errno.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/sys/types.h\
	$(INC)/sys/stat.h\
	$(INC)/sys/statvfs.h\
	$(INC)/sys/mnttab.h\
	$(INC)/sys/param.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/vfstab.h
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o $@ $@.c $(ROOTLIBS) $(LDLIBS)

clobber_df:
	rm -f df

install_df: df
	-rm -f $(OLDBINDIR)/df
	-rm -f $(BINDIR)/df
	$(INS) -f $(INSDIR) df
	$(INS) -f $(INSDIR2) df
# the next line should be taken out after load S18
	-rm -f $(INSDIR2)/devnm
	cp $(INSDIR)/df $(BINDIR)/devnm

#
# generic ncheck
#

ncheck:	ncheck.c\
	$(INC)/stdio.h\
	$(INC)/limits.h\
	$(INC)/fcntl.h\
	$(INC)/sys/types.h\
	$(INC)/sys/vfstab.h\
	$(INC)/sys/wait.h\
	$(INC)/string.h
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o $@ $@.c $(ROOTLIBS) $(LDLIBS)

clobber_ncheck:
	rm -f ncheck

install_ncheck: ncheck
	-rm -f $(OLDDIR)/ncheck
	-rm -f $(INSDIR2)/ncheck
	$(INS) -f $(INSDIR2) ncheck

#
# generic ff
#
ff:	ff.c\
	$(INC)/stdio.h\
	$(INC)/limits.h\
	$(INC)/string.h\
	$(INC)/sys/fstyp.h\
	$(INC)/sys/errno.h\
	$(INC)/sys/vfstab.h
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o $@ $@.c $(ROOTLIBS) $(LDLIBS)

clobber_ff:
	rm -f ff

install_ff: ff 
	-rm -f $(INSDIR2)/ff
	-rm -f $(OLDDIR)/ff
	$(INS) -f $(INSDIR2) ff

# generic fsck
fsck:	fsck.c\
	$(INC)/stdio.h\
	$(INC)/limits.h\
	$(INC)/priv.h\
	$(INC)/mac.h\
	$(INC)/sys/errno.h\
	$(INC)/sys/types.h\
	$(INC)/sys/vfstab.h
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o $@ $@.c $(ROOTLIBS) $(LDLIBS)

clobber_fsck:
	rm -f fsck

install_fsck: fsck
	-rm -f $(OLDDIR)/fsck 
	-rm -f $(INSDIR)/fsck
	-rm -f $(INSDIR2)/fsck
	$(INS) -f $(INSDIR) fsck
	$(INS) -f $(INSDIR2) fsck
	$(INS) -f $(OLDDIR) fsck

# generic volcopy
volcopy: volcopy.c
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o $@ $@.c $(LDLIBS) $(ROOTLIBS)

clobber_volcopy:
	rm -f volcopy

install_volcopy: volcopy
	-rm -f  $(OLDDIR)/volcopy 
	-rm -f  $(INSDIR2)/volcopy 
	$(INS) -f $(INSDIR2) volcopy

#
#  This is for the switchout
#

switch: switchout.o
	$(CC) $(LDFLAGS) -o $@ switchout.o $(ROOTLIBS) $(LDLIBS)

switchout.o:	switchout.c\
	$(INC)/sys/types.h\
	$(INC)/sys/stat.h\
	$(INC)/stdio.h\
	$(INC)/sys/fcntl.h\
	$(INC)/sys/fstyp.h\
	$(INC)/sys/errno.h\
	$(INC)/limits.h

clobber_switch:
	rm -f switch fsdb switchout

install_switch: switch
	-rm -f fsdb
	-rm -f switchout
	-rm -f $(OLDDIR)/fsdb 
	-rm -f $(INSDIR)/fsdb
	-rm -f $(INSDIR2)/fsdb
	ln switch switchout
	ln switch fsdb
	$(INS) -f $(INSDIR) fsdb
	$(INS) -f $(INSDIR2) fsdb
	$(INS) -f $(OLDDIR) fsdb
	-rm -f $(INSDIR2)/switchout
	$(INS) -f $(INSDIR2) switchout
	-rm -f $(OLDDIR)/labelit 
	-rm -f $(INSDIR2)/labelit
	ln $(INSDIR2)/fsdb $(INSDIR2)/labelit
	-rm -f $(OLDDIR)/mkfs 
	-rm -f $(INSDIR)/mkfs
	-rm -f $(INSDIR2)/mkfs
	ln $(INSDIR)/fsdb $(INSDIR)/mkfs
	$(INS) -f $(INSDIR2) $(INSDIR)/mkfs
	-rm -f $(OLDDIR)/dcopy 
	-rm -f $(INSDIR2)/dcopy
	ln $(INSDIR2)/fsdb $(INSDIR2)/dcopy

#
# Generic mklost+found
#
mklost+found: lost+found.sh
	cp lost+found.sh mklost+found 
	chmod 555 mklost+found

clobber_mklost+found: mklost+found
	rm -f mklost+found

install_mklost+found: mklost+found
	-rm -f  $(OLDDIR)/mklost+found
	-rm -f  $(INSDIR2)/mklost+found
	$(INS) -f $(INSDIR2) mklost+found
