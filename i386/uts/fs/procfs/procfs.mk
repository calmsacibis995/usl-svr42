#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/procfs/procfs.mk	1.5"
#ident "$Header: procfs.mk 1.2 91/04/03 $"

include $(UTSRULES)

KBASE     = ../..
INSPERM   = -m 644 -u $(OWN) -g $(GRP)
FS        = $(CONF)/pack.d/procfs/Driver.o
FILES = prioctl.o\
	prmachdep.o\
	prsubr.o\
	prusrio.o\
	prvfsops.o\
	prvnops.o

CFILES = $(FILES:.o=.c)


all:	ID $(FS)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#
ID:
	cd procfs.cf; $(IDINSTALL) -R$(CONF) -M procfs


clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d procfs


lint:
	lint -u -x $(DEFLIST) $(CFLAGS) \
		prioctl.c prmachdep.c prsubr.c prusrio.c prvfsops.c prvnops.c

headinstall: \
	$(KBASE)/fs/procfs/procfs.h \
	$(KBASE)/fs/procfs/prdata.h \
	$(FRC)
	[ -d $(INC)/fs ] || mkdir $(INC)/fs
	[ -d $(INC)/fs/procfs ] || mkdir $(INC)/fs/procfs
	$(INS) -f $(INC)/sys       $(INSPERM) $(KBASE)/fs/procfs/procfs.h
	$(INS) -f $(INC)/fs/procfs $(INSPERM) $(KBASE)/fs/procfs/prdata.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

