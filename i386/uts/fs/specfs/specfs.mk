#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/specfs/specfs.mk	1.6"
#ident "$Header: specfs.mk 1.1 91/03/21 $"

include $(UTSRULES)

KBASE    = ../..
FS	 = $(CONF)/pack.d/specfs/Driver.o
FILES    = \
	specgetsz.o \
	specsubr.o \
	specvfsops.o \
	specvnops.o \
	specsec.o

CFILES = $(FILES:.o=.c)


all:	ID $(FS)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#
ID:
	cd specfs.cf; $(IDINSTALL) -R$(CONF) -M specfs


clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d specfs

headinstall: \
	$(KBASE)/fs/specfs/snode.h \
	$(KBASE)/fs/specfs/devmac.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	[ -d $(INC)/sys/fs ] || mkdir $(INC)/sys/fs
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/specfs/snode.h
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/specfs/devmac.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

