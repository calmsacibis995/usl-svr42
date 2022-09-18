#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/fs.mk	1.15"
#ident "$Header: fs.mk 1.2 91/03/20 $"

include $(UTSRULES)

KBASE     = ..
FS        = $(CONF)/pack.d/fs/Driver.o
FILES = \
	bio.o \
	fbio.o \
	fio.o \
	flock.o \
	dnlc.o \
	fs_subr.o \
	fsflush.o \
	lookup.o \
	pathname.o \
	pipe.o \
	vfs.o \
	vncalls.o \
	fsinode.o \
	vnode.o

CFILES = $(FILES:.o=.c)


all:	ID $(FS) dir 

ID:
	cd fs.cf; $(IDINSTALL) -R$(CONF) -M fs

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

dir:
	@for i in `ls`;\
	do\
		if [ -d $$i -a $$i != fs.cf ];then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk" ; \
			$(MAKE) -f $$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done

depend:: makedep
	@for i in `ls`;\
	do\
		if [ -d $$i -a $$i != fs.cf ];then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk depend" ; \
			$(MAKE) -f $$i.mk depend MAKEFILE=$$i.mk $(MAKEARGS) ; \
			cd .. ; \
		fi;\
	done

clean:
	-rm -f $(FILES)
	@for i in `ls`; \
	do \
		if [ -d $$i -a $$i != fs.cf ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk clean $(MAKEARGS); \
			cd ..; \
		fi; \
	done

clobber:	clean
	@for i in `ls`; \
	do \
		if [ -d $$i -a $$i != fs.cf ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk clobber $(MAKEARGS); \
			cd ..; \
		fi; \
	done
	-$(IDINSTALL) -e -R$(CONF) -d fs

Header = \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/dir.h \
	$(KBASE)/fs/dirent.h \
	$(KBASE)/fs/dnlc.h \
	$(KBASE)/fs/fblk.h \
	$(KBASE)/fs/fbuf.h \
	$(KBASE)/fs/fcntl.h \
	$(KBASE)/fs/file.h \
	$(KBASE)/fs/filio.h \
	$(KBASE)/fs/filsys.h \
	$(KBASE)/fs/flock.h \
	$(KBASE)/fs/fs_subr.h \
	$(KBASE)/fs/fsid.h \
	$(KBASE)/fs/fstyp.h \
	$(KBASE)/fs/ino.h \
	$(KBASE)/fs/ioccom.h \
	$(KBASE)/fs/fsinode.h \
	$(KBASE)/fs/mkfs.h \
	$(KBASE)/fs/mntent.h \
	$(KBASE)/fs/mnttab.h \
	$(KBASE)/fs/mode.h \
	$(KBASE)/fs/mount.h \
	$(KBASE)/fs/pathname.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/fs/stat.h \
	$(KBASE)/fs/statfs.h \
	$(KBASE)/fs/statvfs.h \
	$(KBASE)/fs/ustat.h \
	$(KBASE)/fs/vfs.h \
	$(KBASE)/fs/vfstab.h \
	$(KBASE)/fs/vnode.h

headinstall: $(Header) $(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	@for file in $(Header);\
	do\
		$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $$file; \
	done
	@for i in `ls`; \
	do \
		if [ -d $$i -a $$i != fs.cf ];then\
			cd $$i; \
			$(MAKE) -f $$i.mk headinstall $(MAKEARGS); \
			cd ..; \
		fi; \
	done


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

