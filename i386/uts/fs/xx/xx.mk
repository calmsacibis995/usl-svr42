#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/xx/xx.mk	1.8"
#ident "$Header: $"

include $(UTSRULES)

KBASE    = ../..
FS	 = $(CONF)/pack.d/XENIX/Driver.o
FILES = \
	xxalloc.o \
	xxblklist.o \
	xxbmap.o \
	xxdir.o \
	xxsearch.o \
	xxgetsz.o \
	xxinode.o \
	xxrdwri.o \
	xxvfsops.o \
	xxvnops.o

CFILES =  \
	xxalloc.c  \
	xxblklist.c  \
	xxbmap.c  \
	xxdir.c  \
	xxgetsz.c  \
	xxinode.c  \
	xxrdwri.c  \
	xxvfsops.c  \
	xxvnops.c


SFILES =  \
	xxsearch.s 


all:	ID $(FS)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#
ID:
	cd xx.cf; $(IDINSTALL) -R$(CONF) -M XENIX


clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d XENIX

headinstall: \
	$(KBASE)/fs/xx/xxfblk.h \
	$(KBASE)/fs/xx/xxfilsys.h \
	$(KBASE)/fs/xx/inode.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	[ -d $(INC)/sys/fs ] || mkdir $(INC)/sys/fs
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/xx/xxfblk.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/xx/xxfilsys.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/xx/inode.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

