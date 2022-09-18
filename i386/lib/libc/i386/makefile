#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-i386:makefile	1.44.1.26"
#
#  makefile for libc/i386
#
#
# The variable PROF is null by default, causing profiled object to be
# maintained.  If profiled object is not desired, the reassignment
# PROF=@# should appear in the make command line.
#

include $(LIBRULES)

.SUFFIXES: .p .P .A

PROF=
NONPROF=
PIC=
ABI=
INCLIBC=../inc
DEFLIST=
INCLIST=-I$(INCLIBC)
VERSDEF=ansi.def
M4DEFS=m4.def sys.def $(VERSDEF)
AWK=awk

OBJECTS=\
crt/cerror.o      crt/fpreal.o      crt/mcount.o      \
csu/crt0.o        csu/crt1.o        csu/crti.o        csu/crtn.o        \
csu/mcrt0.o       csu/mcrt1.o       csu/pcrt1.o       \
csu/pcrti.o       csu/pcrtn.o       \
fp/finite.o       fp/finitel.o	    fp/fp_data.o      fp/fpgetmask.o    \
fp/fpgetround.o   fp/fpgetsticky.o  fp/fpsetmask.o    fp/fpsetround.o   \
fp/fpsetsticky.o  fp/fpstart.o      fp/fpstart0.o     fp/fpstart1.o     \
fp/logb.o         fp/logbl.o 	    fp/scalb.o        fp/scalbl.o	\
gen/abs.o         gen/atof.o        gen/biglitpow.o   gen/cuexit.o      \
gen/dtop.o        gen/ecvt.o        gen/cvtl.o	\
gen/fk_ctype.o	  gen/fk_iob.o	    \
gen/fpgetrnd.o    gen/fpsetrnd.o    gen/frexp.o       gen/frexpl.o	\
gen/gen_def.o     gen/getctxt.o     gen/i386_data.o   \
gen/isnand.o      gen/isnanf.o      gen/isnanl.o      gen/l3.o		\
gen/ladd.o        gen/lconstants.o  gen/ldexp.o       gen/ldexpl.o	\
gen/ldivide.o     gen/lexp10.o      gen/limits.o      gen/llog10.o      \
gen/lmul.o        gen/lshiftl.o     gen/lsign.o       gen/lsub.o        \
gen/ltostr.o      gen/makectxt.o    gen/mall_data.o   gen/memccpy.o     \
gen/memchr.o      gen/memcmp.o      gen/memcpy.o      gen/memset.o      \
gen/ptod.o        gen/setjmp.o      gen/sh_data.o     \
gen/siginfolst.o  gen/siglongjmp.o  gen/sigsetjmp.o   \
gen/strcat.o      gen/strchr.o      gen/strcmp.o      gen/strcpy.o      \
gen/strlen.o      gen/strncat.o     gen/strncmp.o     gen/strncpy.o     \
gen/strrchr.o     gen/strtod.o      gen/strtold.o     gen/swapctxt.o    \
sys/_nfssys.o     sys/_rename.o     sys/_sigaction.o  sys/_signal.o	\
sys/access.o      sys/acct.o        sys/acl.o	      sys/aclipc.o	\
sys/adjtime.o     sys/alarm.o       sys/auditbuf.o    sys/auditctl.o    \
sys/auditdmp.o    sys/auditevt.o    sys/auditlog.o    \
sys/chdir.o       sys/chmod.o       \
sys/chown.o       sys/chroot.o      sys/close.o       sys/creat.o       \
sys/devstat.o     sys/dup.o         sys/execl.o       sys/execle.o      \
sys/exect.o       sys/execv.o       sys/execve.o      sys/exit.o        \
sys/fchdir.o      sys/fchmod.o      sys/fchown.o      sys/fcntl.o       \
sys/fdevstat.o    sys/filepriv.o    sys/flvlfile.o    sys/fork.o        \
sys/fpathconf.o   sys/fstat.o       \
sys/fstatf.o      sys/fstatfs.o     sys/fstatvfs.o    \
sys/fsync.o       sys/fxstat.o      \
sys/getdents.o    sys/getegid.o     sys/geteuid.o     \
sys/getgid.o      sys/getgroups.o   sys/getksym.o     sys/getmsg.o      \
sys/getpid.o      sys/getpmsg.o     sys/getppid.o     \
sys/getrlimit.o   sys/getuid.o      sys/gtty.o        \
sys/ioctl.o       sys/kill.o        sys/lchown.o      \
sys/link.o        sys/lseek.o       sys/lstat.o       sys/lvldom.o	\
sys/lvlequal.o	  sys/lvlfile.o	    sys/lvlipc.o      sys/lvlproc.o	\
sys/lvlvfs.o      sys/lxstat.o      \
sys/memcntl.o     sys/mincore.o     \
sys/mkdir.o       sys/mkmld.o	    sys/mknod.o       sys/mldmode.o	\
sys/mmap.o        sys/modadm.o	    sys/modload.o	\
sys/modpath.o	  sys/modstat.o	    sys/moduload.o    sys/mount.o       \
sys/mprotect.o    sys/munmap.o      sys/nice.o        \
sys/nuname.o      sys/open.o        sys/pathconf.o    sys/procpriv.o	\
sys/pause.o       sys/pipe.o        \
sys/plock.o       sys/poll.o        sys/priocntlset.o sys/profil.o      \
sys/ptrace.o      sys/putmsg.o      sys/putpmsg.o     \
sys/read.o        sys/readksym.o    sys/readlink.o    sys/readv.o       \
sys/rfsys.o       sys/rmdir.o       sys/sbrk.o        sys/secadvise.o \
sys/secsys.o	  sys/seteguid.o    \
sys/setgid.o      sys/setgroups.o   sys/setpgrp.o     sys/setrlimit.o   \
sys/setsid.o      sys/setuid.o      sys/setuname.o    \
sys/sigaction.o   sys/sigaltstk.o   sys/signal.o      sys/sigpending.o  \
sys/sigprocmsk.o  sys/sigsendset.o  sys/sigsuspend.o  \
sys/stat.o        sys/statf.o       sys/statfs.o      sys/statvfs.o     \
sys/stime.o       sys/stty.o        sys/symlink.o     \
sys/sync.o        sys/syscall.o     sys/sysconfig.o   sys/sysfs.o       \
sys/sysi86.o      sys/sysinfo.o     sys/time.o        sys/times.o       \
sys/uadmin.o      sys/ucontext.o    sys/ulimit.o      sys/umask.o       \
sys/umount.o      sys/uname.o       sys/unlink.o      \
sys/ustat.o       sys/utime.o       sys/utssys.o      sys/vfork.o       \
sys/wait.o        sys/waitid.o      sys/write.o       sys/writev.o      \
sys/xmknod.o      sys/xstat.o

POBJECTS=$(OBJECTS:.o=.p)

PIC_OBJECTS=\
crt/cerror.P      \
fp/fpstart.P      fp/fpstart1.P     fp/logb.P         fp/scalb.P        \
fp/fp_data.P	\
gen/atof.P	  gen/biglitpow.P   gen/cuexit.P      gen/cvtl.P	\
gen/dtop.P        gen/ecvt.P        gen/frexp.P       gen/getctxt.P     \
gen/isnand.P      gen/isnanl.P	    gen/ldexp.P       gen/limits.P      \
gen/l3.P	\
gen/ltostr.P      gen/i386_data.P   gen/memccpy.P     gen/memchr.P      \
gen/memcmp.P      gen/memcpy.P      gen/memset.P      gen/ptod.P        \
gen/setjmp.P      gen/sh_data.P     gen/siglongjmp.P  gen/sigsetjmp.P   \
gen/strcat.P      gen/strchr.P      gen/strcmp.P      gen/strcpy.P      \
gen/strlen.P      gen/strncat.P     gen/strncmp.P     gen/strncpy.P     \
gen/strrchr.P     gen/strtod.P      gen/strtold.P     \
sys/_rename.P     sys/_sigaction.P  sys/_signal.P     sys/access.P      \
sys/acct.P        sys/acl.P	    sys/aclipc.P      sys/alarm.P       \
sys/auditbuf.P	  sys/auditctl.P    sys/auditdmp.P    sys/auditevt.P	\
sys/auditlog.P    sys/chdir.P       sys/chmod.P       \
sys/chown.P       sys/chroot.P      sys/close.P       sys/creat.P       \
sys/devstat.P     \
sys/dup.P         sys/execl.P       sys/execle.P      sys/execv.P       \
sys/execve.P      sys/exit.P        sys/fchdir.P      sys/fchmod.P      \
sys/fchown.P      sys/fcntl.P       sys/fdevstat.P    sys/filepriv.P	\
sys/flvlfile.P    sys/fork.P        \
sys/fpathconf.P   sys/fstat.P       \
sys/fstatvfs.P    sys/fsync.P       sys/fxstat.P      sys/getdents.P    \
sys/getegid.P     sys/geteuid.P     sys/getgid.P      sys/getgroups.P   \
sys/getmsg.P      sys/getpid.P      sys/getpmsg.P     \
sys/getppid.P     \
sys/getrlimit.P   sys/getuid.P      sys/ioctl.P       sys/kill.P        \
sys/lchown.P      sys/link.P        sys/lseek.P       sys/lstat.P       \
sys/lvldom.P	  sys/lvlequal.P    sys/lvlfile.P     sys/lvlipc.P	\
sys/lvlproc.P	  sys/lvlvfs.P      sys/lxstat.P      sys/memcntl.P     \
sys/mincore.P     sys/mkdir.P       sys/mkmld.P	      sys/mknod.P       \
sys/mldmode.P	  sys/mmap.P        sys/mount.P       \
sys/mprotect.P    sys/munmap.P      sys/nice.P        \
sys/nuname.P      sys/open.P        sys/pathconf.P    sys/procpriv.P	\
sys/pause.P       sys/pipe.P        \
sys/plock.P       sys/poll.P        sys/priocntlset.P sys/profil.P      \
sys/ptrace.P      sys/putmsg.P      sys/putpmsg.P     \
sys/read.P        sys/readlink.P    sys/readv.P       \
sys/rmdir.P       sys/secadvise.P   sys/secsys.P      sys/sbrk.P        \
sys/seteguid.P	  sys/setgid.P      \
sys/setgroups.P   sys/setpgrp.P     sys/setrlimit.P   sys/setsid.P      \
sys/setuid.P      sys/sigaction.P   sys/sigaltstk.P   \
sys/signal.P      sys/sigpending.P  sys/sigprocmsk.P  sys/sigsendset.P  \
sys/sigsuspend.P  sys/stat.P        sys/statvfs.P     sys/stime.P       \
sys/symlink.P     sys/sync.P        sys/syscall.P     \
sys/sysconfig.P   sys/sysi86.P      sys/sysinfo.P     \
sys/time.P        sys/times.P       sys/uadmin.P      \
sys/ucontext.P    sys/ulimit.P      sys/umask.P       sys/umount.P      \
sys/uname.P       sys/unlink.P      sys/ustat.P       sys/utime.P       \
sys/vfork.P       \
sys/wait.P        sys/waitid.P      sys/write.P       sys/writev.P      \
sys/xmknod.P      sys/xstat.P

ABI_OBJECTS=$(PIC_OBJECTS:.P=.A)


all:
	if [ "$(ISANSI)" = "TRUE" ] ; \
	then VERSDEF="ansi.def"; \
	else VERSDEF="nonansi.def"; \
	fi ; \
	$(MAKE) -e all_vers VERSDEF=$$VERSDEF

all_vers:
	$(PROF)$(MAKE) -e $(IGN) prof NONPROF=@# PIC=@# ABI=@#
	$(PIC)$(MAKE) -e $(IGN) pic NONPROF=@# PROF=@# ABI=@#
	$(ABI)$(MAKE) -e $(IGN) abi NONPROF=@# PROF=@# PIC=@#
	$(NONPROF)$(MAKE) -e $(IGN) nonprof PROF=@# PIC=@# ABI=@#

nonprof: $(OBJECTS)

prof: $(POBJECTS)

pic: $(PIC_OBJECTS)

abi: $(ABI_OBJECTS)

.c.o .c.p .c.P .c.A:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS)  -c $*.c
	$(NONPROF)mv $(*F).o $*.o
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) -c -p $*.c  && mv $(*F).o $*.p
	$(PIC)@echo $*.c:
	$(PIC)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) -c -DDSHLIB -Kpic $*.c  && mv $(*F).o $*.P
	$(ABI)@echo $*.c:
	$(ABI)$(CC) $(DEFLIST) $(INCLIST) $(CFLAGS) -c -DABI -DDSHLIB -Kpic $*.c  && mv $(*F).o $*.A

.s.o .s.p .s.P .s.A:
	$(NONPROF)@echo $*.s:
	$(NONPROF)$(AS) -o $*.o -m -- $(M4DEFS) -DMCOUNT=\# nonpic.def $*.s 
	$(PROF)@echo $*.s:
	$(PROF)$(AS) -o $*.p -m -- $(M4DEFS) mcount.def nonpic.def $*.s 
	$(PIC)@echo $*.s:
	$(PIC)$(AS) -o $*.P -m -- -DDSHLIB $(M4DEFS) -DMCOUNT=\# pic.def $*.s
	$(ABI)@echo $*.s:
	$(ABI)$(AS) -o $*.A -m -- -DABI -DDSHLIB $(M4DEFS) -DMCOUNT=\# pic.def $*.s 

archive:
	#
	# Note that "archive" is invoked with libc/object as current directory.
	#
	# fk_ctype.o and fk_iob.o are only needed with the shared library
	rm -f fk_ctype.o fk_ctype.p fk_iob.o fk_iob.p
	#
	# figure out the correct ordering for all the archive modules
	mv cuexit.o cuexit.x
	mv fpstart.o fpstart.x
	mv fpstart0.o fpstart0.x
	mv signal.o signal.x
	mv _signal.o _signal.x
	mv kill.o kill.x
	mv getpid.o getpid.x
	mv cerror.o cerror.x
	mv fpsetmask.o fpsetmask.x
	mv atexit.o atexit.x
	-rm -f old_toupper.o old_tolower.o
	ls | grep '\.o$$' | xargs $(LORDER) | $(TSORT) >objlist
	mv cuexit.x cuexit.o
	mv fpstart.x fpstart.o
	mv fpstart0.x fpstart0.o
	mv signal.x signal.o
	mv _signal.x _signal.o
	mv kill.x kill.o
	mv getpid.x getpid.o
	mv cerror.x cerror.o
	mv fpsetmask.x fpsetmask.o
	mv atexit.x atexit.o
	#
	# build the archive with the modules in correct order.
	$(AR) q ../lib.libc atexit.o
	xargs $(AR) q ../lib.libc <objlist
	$(AR) q ../lib.libc signal.o _signal.o fpstart.o fpstart0.o \
	fpsetmask.o kill.o getpid.o \
	cerror.o cuexit.o
	if [ "$(MAC)" = BMAUMAC ] ; then \
		$(AR) d ../lib.libc fpstart0.o ; \
	fi
	$(PROF)#
	$(PROF)# build the profiled library archive, first renaming the
	$(PROF)#	.p (profiled object) modules to .o
	$(PROF)find . -name '*.p' -print | while read i; do mv $$i `basename $$i .p`.o ; done
	if [ "$(PROF)" != "@#" ]; \
	then \
	$(PROF)$(AR) q ../libp.libc atexit.o; \
	$(PROF)xargs $(AR) q ../libp.libc <objlist; \
	$(PROF)$(AR) q ../libp.libc signal.o _signal.o fpstart.o fpstart0.o \
	fpsetmask.o kill.o getpid.o cerror.o cuexit.o ; \
	if [ "$(MAC)" = BMAUMAC ] ; then \
		$(AR) d ../libp.libc fpstart0.o ; \
	fi \
	fi

shared:
	#
	# Note that "shared" is invoked with libc/object as current directory
	#
	-rm -rf shared
	mkdir shared
	cat ../$(CPU)/shared_objects | grep -v "\#"  | \
	xargs sh -sc 'cp "$$@" shared'
	cat ../$(CPU)/shared_objects | grep "^[^\.\#]" | sed 's/\.P/.o/' | \
	xargs sh -sc 'rm -f "$$@"'
	#
	# create the shared portion of the library
	cd shared; \
	grep -v "#" ../../$(CPU)/shared_objects | xargs echo $(LD) -r -o libc_TMP.'$$i' > libcbuild; \
	i=0; \
	while read cmd; \
	do \
	eval "$$cmd"; \
	i=`expr "$$i" + 1`; \
	done < libcbuild; \
	$(LD) -o ../../libc.so.1 -dy -G -e _rt_boot -h /usr/lib/libc.so.1 -Bsymbolic=`grep -v "#" ../../i386/Nbsymbs | awk ' { printf } ' ` libc_TMP.*; \
	rm -f libcbuild libc_TMP.*
	#
	# remove extraneous rtld file
	-rm rtabiaddr.o
	# figure out the correct ordering for all the archive modules
	mv cerror.o cerror.x
	mv fpstart0.o fpstart0.x
	mv fpsetmask.o fpsetmask.x
	-rm -f old_toupper.o old_tolower.o
	ls | grep '\.o$$' | xargs $(LORDER) | $(TSORT) >objlist
	mv cerror.x cerror.o
	mv fpstart0.x fpstart0.o
	mv fpsetmask.x fpsetmask.o
	#
	# build the archive with the modules in correct order.
	xargs $(AR) q ../libc.so <objlist
	$(AR) q ../libc.so cerror.o ../libc.so.1 \
		fpstart0.o fpsetmask.o
	if [ "$(MAC)" = BMAUMAC ] ; then \
		$(AR) d ../libc.so fpstart0.o ; \
	fi

abi_lib: 
	#
	# Note that "abi" is invoked with libc/object as current directory
	#
	# fk_iob.o is only needed with the default shared library
	rm -f fk_iob.o fk_iob.p
	#
	-rm -rf shared
	mkdir shared
	cat ../$(CPU)/abi_objects | grep -v "\#"  | \
	xargs sh -sc 'cp "$$@" shared'
	cat ../$(CPU)/abi_objects | grep "^[^\.\#]" | sed 's/\.A/.o/' | \
	xargs sh -sc 'rm -f "$$@"'
	#
	# create the shared portion of the library
	cd shared; \
	grep -v "#" ../../$(CPU)/abi_objects | xargs echo $(LD) -r -o ld_TMP.'$$i' > ldbuild; \
	i=0; \
	while read cmd; \
	do \
	eval "$$cmd"; \
	i=`expr "$$i" + 1`; \
	done < ldbuild; \
	$(LD) -o ../../ld.so.1 -dy -G -e _rt_boot -h /usr/lib/ld.so.1 -ztext -zdefs -Bsymbolic=`grep -v "#" ../../i386/Nbsymbs | awk ' { printf } ' ` ld_TMP.*; \
	rm -f ldbuild ld_TMP.*
	#
	# figure out the correct ordering for all the archive modules
	mv cerror.o cerror.x
	mv fpstart0.o fpstart0.x
	mv fpsetmask.o fpsetmask.x
	-rm -f old_toupper.o old_tolower.o
	ls | grep '\.o$$' | xargs $(LORDER) | $(TSORT) >objlist
	mv cerror.x cerror.o
	mv fpstart0.x fpstart0.o
	mv fpsetmask.x fpsetmask.o
	#
	# build the archive with the modules in correct order.
	xargs $(AR) q ../libabi.so <objlist
	$(AR) q ../libabi.so cerror.o ../ld.so.1 \
		fpstart0.o fpsetmask.o
	if [ "$(MAC)" = BMAUMAC ] ; then \
		$(AR) d ../libabi.so fpstart0.o ; \
	fi

compat_lib:
	#build the shared object libc.so.1.1 which contains libc functions 
	#only. It is used for compatibilty with systems supporting the
	#two library approach for dynamic linking.
	-rm -rf shared
	mkdir shared
	cat ../$(CPU)/compat_objects | grep -v "\#"  | \
	xargs sh -sc 'cp "$$@" shared'
	cat ../$(CPU)/compat_objects | grep "^[^\.\#]" | sed 's/\.P/.o/' | \
	xargs sh -sc 'rm -f "$$@"'
	#
	# create the shared library
	cd shared; \
	grep -v "#" ../../$(CPU)/compat_objects | xargs echo $(LD) -r -o compat_TMP.'$$i' > compatbuild; \
	i=0; \
	while read cmd; \
	do \
	eval "$$cmd"; \
	i=`expr "$$i" + 1`; \
	done < compatbuild; \
	$(LD) -o ../../libc.so.1.1 -dy -G -h /usr/lib/libc.so.1.1 compat_TMP.*; \
	rm -f compatbuild compat_TMP.*
	#
clean:
	-rm -f *.o
	-rm -f *.p
	-rm -f *.P
	-rm -f *.A
	-rm -f *.s

clobber: clean
	-rm -f crt/*.o
	-rm -f csu/*.o
	-rm -f fp/*.o
	-rm -f gen/*.o
	-rm -f sys/*.o
	-rm -f crt/*.p
	-rm -f csu/*.p
	-rm -f fp/*.p
	-rm -f gen/*.p
	-rm -f sys/*.p
	-rm -f */*.P
	-rm -f */*.A
