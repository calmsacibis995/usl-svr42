#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/bfs/bfs.mk	1.4"
#ident "$Header: bfs.mk 1.2 91/03/20 $"

include $(UTSRULES)

KBASE     = ../..
FS        = $(CONF)/pack.d/bfs/Driver.o
INSPERM   = -m 644 -u $(OWN) -g $(GRP)

FILES =\
	bfs_compact.o \
	bfs_subr.o \
	bfs_vfsops.o \
	bfs_vnops.o

CFILES = $(FILES:.o=.c)


all:	ID $(FS)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)


#
# Configuration Section
#
ID:
	cd bfs.cf; $(IDINSTALL) -R$(CONF) -M bfs


clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d bfs

headinstall: \
	$(KBASE)/fs/bfs/bfs.h \
	$(KBASE)/fs/bfs/bfs_compact.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	[ -d $(INC)/sys/fs ] || mkdir $(INC)/sys/fs
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/bfs/bfs.h
	$(INS) -f $(INC)/sys/fs $(INSPERM) $(KBASE)/fs/bfs/bfs_compact.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

