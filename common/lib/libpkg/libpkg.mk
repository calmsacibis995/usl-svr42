#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libpkg:common/lib/libpkg/libpkg.mk	1.13.14.1"
#ident	"$Header: $"


include $(LIBRULES)

#	Makefile for libpkg

LOCALINC= -Ihdrs

MAKEFILE = libpkg.mk

LIBRARY = libpkg.a

OBJECTS =  canonize.o ckparam.o ckvolseq.o cvtpath.o devtype.o dstream.o \
	gpkglist.o gpkgmap.o isdir.o logerr.o mappath.o pkgexecl.o \
	pkgexecv.o pkgmount.o pkgtrans.o pkgxpand.o ppkgmap.o privent.o \
	progerr.o putcfile.o rrmdir.o runcmd.o srchcfile.o tputcfent.o verify.o

SOURCES =  canonize.c ckparam.c ckvolseq.c cvtpath.c devtype.c dstream.c \
	gpkglist.c gpkgmap.c isdir.c logerr.c mappath.c pkgexecl.c pkgexecv.c \
	pkgmount.c pkgtrans.c pkgxpand.c ppkgmap.c privent.c progerr.c \
	putcfile.c rrmdir.c runcmd.c srchcfile.c tputcfent.c verify.c

all:		$(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)


canonize.o canonize.o:	 $(INC)/string.h 


ckvolseq.o ckvolseq.o:	 $(INC)/limits.h \
		 $(INC)/pkgstrct.h $(INC)/stdio.h \
		 $(INC)/sys/select.h $(INC)/sys/types.h 

ckparam.o ckparam.o:	$(INC)/ctype.h \
		 $(INC)/string.h $(INC)/sys/types.h


cvtpath.o cvtpath.o:	 $(INC)/string.h 


devtype.o devtype.o:	 $(INC)/pkgdev.h \
		 $(INC)/stdio.h $(INC)/string.h 


dstream.o dstream.o:	 $(INC)/fcntl.h \
		 $(INC)/signal.h $(INC)/stdio.h \
		 $(INC)/string.h $(INC)/sys/fcntl.h	\
		 $(INC)/sys/select.h $(INC)/sys/signal.h \
		 $(INC)/sys/statfs.h $(INC)/sys/types.h 


gpkglist.o gpkglist.o:	 $(INC)/ctype.h \
		 $(INC)/errno.h $(INC)/pkginfo.h \
		 $(INC)/signal.h $(INC)/stdio.h \
		 $(INC)/string.h $(INC)/sys/errno.h	\
		 $(INC)/sys/select.h $(INC)/sys/signal.h \
		 $(INC)/sys/types.h $(INC)/valtools.h 


gpkgmap.o gpkgmap.o:	 $(INC)/ctype.h \
		 $(INC)/limits.h $(INC)/pkgstrct.h \
		 $(INC)/stdio.h $(INC)/string.h \
		 $(INC)/sys/select.h $(INC)/sys/types.h 


isdir.o isdir.o:	 $(INC)/limits.h \
		 $(INC)/sys/select.h $(INC)/sys/stat.h \
		 $(INC)/sys/time.h $(INC)/sys/types.h \
		 $(INC)/time.h 


logerr.o logerr.o:	 $(INC)/stdio.h \
		 $(INC)/string.h $(INC)/varargs.h 


mappath.o mappath.o:	 $(INC)/ctype.h \
		 $(INC)/limits.h $(INC)/string.h 


pkgexecl.o pkgexecl.o:	 $(INC)/signal.h \
		 $(INC)/stdio.h $(INC)/string.h \
		 $(INC)/sys/select.h $(INC)/sys/signal.h \
		 $(INC)/sys/types.h $(INC)/varargs.h 


pkgexecv.o pkgexecv.o:	 $(INC)/signal.h \
		 $(INC)/stdio.h $(INC)/string.h \
		 $(INC)/sys/select.h $(INC)/sys/signal.h \
		 $(INC)/sys/types.h 


pkgmount.o pkgmount.o:	 $(INC)/devmgmt.h	\
		 $(INC)/pkgdev.h $(INC)/pkginfo.h \
		 $(INC)/stdio.h $(INC)/string.h \
		 $(INC)/sys/mount.h $(INC)/sys/select.h \
		 $(INC)/sys/types.h 


pkgtrans.o pkgtrans.o:	 $(INC)/ctype.h \
		 $(INC)/dirent.h $(INC)/fcntl.h \
		 $(INC)/limits.h $(INC)/pkgdev.h \
		 $(INC)/pkginfo.h	$(INC)/pkgstrct.h	\
		 $(INC)/pkgtrans.h $(INC)/signal.h \
		 $(INC)/stdio.h $(INC)/string.h \
		 $(INC)/sys/dirent.h $(INC)/sys/fcntl.h \
		 $(INC)/sys/select.h $(INC)/sys/signal.h \
		 $(INC)/sys/types.h $(INC)/varargs.h 


pkgxpand.o pkgxpand.o:	 $(INC)/limits.h \
		 $(INC)/stdio.h $(INC)/string.h 


ppkgmap.o ppkgmap.o:	 $(INC)/limits.h \
		 $(INC)/pkgstrct.h $(INC)/stdio.h \
		 $(INC)/string.h $(INC)/sys/select.h \
		 $(INC)/sys/types.h 


privent.o privent.o:	$(INC)/stdio.h \
		 $(INC)/string.h $(INC)/priv.h \
		 $(INC)/pkgstrct.h $(INC)/sys/secsys.h \
		 $(INC)/errno.h


progerr.o progerr.o:	 $(INC)/stdio.h \
		 $(INC)/varargs.h	


putcfile.o putcfile.o:	 $(INC)/limits.h \
		 $(INC)/pkgstrct.h $(INC)/stdio.h \
		 $(INC)/string.h $(INC)/sys/select.h \
		 $(INC)/sys/types.h 


rrmdir.o rrmdir.o:	 $(INC)/limits.h 


runcmd.o runcmd.o:	 $(INC)/signal.h \
		 $(INC)/stdio.h $(INC)/string.h \
		 $(INC)/sys/select.h $(INC)/sys/signal.h \
		 $(INC)/sys/types.h 


srchcfile.o	srchcfile.o:	 $(INC)/ctype.h \
		 $(INC)/limits.h $(INC)/pkgstrct.h \
		 $(INC)/stdio.h $(INC)/string.h \
		 $(INC)/sys/select.h $(INC)/sys/types.h 


tputcfent.o	tputcfent.o:	 $(INC)/limits.h \
		 $(INC)/pkgstrct.h $(INC)/stdio.h \
		 $(INC)/string.h $(INC)/sys/select.h \
		 $(INC)/sys/types.h 


verify.o verify.o:	 $(INC)/grp.h \
		 $(INC)/limits.h $(INC)/pkgstrct.h \
		 $(INC)/pwd.h $(INC)/stdio.h \
		 $(INC)/string.h $(INC)/sys/mkdev.h	\
		 $(INC)/sys/select.h $(INC)/sys/stat.h \
		 $(INC)/sys/time.h $(INC)/sys/types.h \
		 $(INC)/sys/utime.h $(INC)/time.h \
		 $(INC)/utime.h $(INC)/varargs.h 

GLOBALINCS = hdrs/sec.h $(INC)/ctype.h $(INC)/devmgmt.h \
	$(INC)/dirent.h $(INC)/errno.h $(INC)/fcntl.h \
	$(INC)/grp.h $(INC)/limits.h $(INC)/pkgdev.h \
	$(INC)/pkginfo.h $(INC)/pkgstrct.h \
	$(INC)/pkgtrans.h $(INC)/pwd.h $(INC)/signal.h \
	$(INC)/stdio.h $(INC)/string.h $(INC)/sys/dirent.h \
	$(INC)/sys/errno.h $(INC)/sys/fcntl.h \
	$(INC)/sys/mkdev.h $(INC)/sys/mount.h \
	$(INC)/sys/select.h $(INC)/sys/signal.h \
	$(INC)/sys/stat.h $(INC)/sys/statfs.h \
	$(INC)/sys/time.h $(INC)/sys/types.h \
	$(INC)/sys/utime.h $(INC)/time.h $(INC)/utime.h \
	$(INC)/valtools.h $(INC)/varargs.h 


clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(LIBRARY)

newmakefile:
	makefile -m -f $(MAKEFILE) -L $(LIBRARY)  -s INC $(INC)

install: all
	$(INS) -f $(USRLIB) -m 644 $(LIBRARY) 

size: all
	$(SIZE) $(LIBRARY)

strip: all

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(USRLIB) | tr ' ' '\012' | sort

product:
	@echo $(LIBRARY)  |  tr ' ' '\012'  | \
	sed 's;^;$(USRLIB)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(LIBRARY)
