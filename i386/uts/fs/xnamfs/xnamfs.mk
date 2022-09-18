#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/xnamfs/xnamfs.mk	1.7"
#ident "$Header: xnamfs.mk 1.1 91/03/21 $"

include $(UTSRULES)

KBASE    = ../..
FS	 = $(CONF)/pack.d/xnamfs/Driver.o
MODSTUB  = $(CONF)/pack.d/xnamfs/Modstub.o
FILES = \
	xnamgetsizes.o \
	xnamsubr.o \
	xnamvfsops.o \
	xnamvnops.o \
	xsd.o \
	xsem.o

CFILES = $(FILES:.o=.c)


all:	ID $(FS) $(MODSTUB)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

$(MODSTUB): xnamfs_stub.o
	$(LD) -r -o $@ xnamfs_stub.o

#
# Configuration Section
#
ID:
	cd xnamfs.cf; $(IDINSTALL) -R$(CONF) -M xnamfs


clean:
	-rm -f $(FILES) xnamfs_stub.o

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d xnamfs


headinstall: \
	$(KBASE)/fs/xnamfs/xnamnode.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	[ -d $(INC)/sys/fs ] || mkdir $(INC)/sys/fs
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/xnamfs/xnamnode.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

