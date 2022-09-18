#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:fs/fifofs/fifofs.mk	1.6"
#ident "$Header: fifofs.mk 1.2 91/03/21 $"

include $(UTSRULES)

KBASE    = ../..
FS	 = $(CONF)/pack.d/fifofs/Driver.o

FILES = \
	fifovnops.o \
	fifosubr.o

CFILES = $(FILES:.o=.c)



all:	ID $(FS)

$(FS):	$(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#
ID:
	cd fifofs.cf; $(IDINSTALL) -R$(CONF) -M fifofs


clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d fifofs


headinstall: \
	$(KBASE)/fs/fifofs/fifonode.h \
	$(FRC)
	[ -d $(INC)/sys ] || mkdir $(INC)/sys
	[ -d $(INC)/sys/fs ] || mkdir $(INC)/sys/fs
	$(INS) -f $(INC)/sys/fs -m 644 -u $(OWN) -g $(GRP) $(KBASE)/fs/fifofs/fifonode.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

