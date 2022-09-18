#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/s5fs/s5fs.mk	1.6"
#ident "$Header: s5fs.mk 1.2 91/03/22 $"

include $(UTSRULES)

KBASE   = ../..
INSPERM = -m 644 -u $(OWN) -g $(GRP)
FS	= $(CONF)/pack.d/s5/Driver.o
FILES = \
	s5alloc.o \
	s5blklist.o \
	s5bmap.o \
	s5dir.o \
	s5getsz.o \
	s5inode.o \
	s5rdwri.o \
	s5search.o \
	s5vfsops.o \
	s5vnops.o \
	s5machdep.o

CFILES =  \
	s5alloc.c  \
	s5blklist.c  \
	s5bmap.c  \
	s5dir.c  \
	s5getsz.c  \
	s5inode.c  \
	s5rdwri.c  \
	s5vfsops.c  \
	s5vnops.c  \
	s5machdep.c


SFILES =  \
	s5search.s 


all:	ID $(FS)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#
ID:
	cd s5.cf; $(IDINSTALL) -R$(CONF) -M s5 


clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d s5

headinstall: \
	$(KBASE)/fs/s5fs/s5dir.h \
	$(KBASE)/fs/s5fs/s5fblk.h \
	$(KBASE)/fs/s5fs/s5filsys.h \
	$(KBASE)/fs/s5fs/s5ino.h \
	$(KBASE)/fs/s5fs/s5inode.h \
	$(KBASE)/fs/s5fs/s5macros.h \
	$(KBASE)/fs/s5fs/s5param.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	[ -d $(INC)/sys/fs ] || mkdir $(INC)/sys/fs
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/s5fs/s5dir.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/s5fs/s5fblk.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/s5fs/s5filsys.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/s5fs/s5ino.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/s5fs/s5inode.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/s5fs/s5macros.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/s5fs/s5param.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/s5fs/s5param.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

