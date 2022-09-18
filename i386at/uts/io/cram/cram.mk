#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/cram/cram.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

KBASE   = ../..
CNFFILE = $(CONF)/pack.d/cram/Driver.o
DFILES  = cram.o

CFILES = $(DFILES:.o=.c)


all:	ID $(CNFFILE)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd cram.cf; $(IDINSTALL) -R$(CONF) -M cram

#
# Header Install Section
#
headinstall: \
	$(KBASE)/io/cram/cram.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/cram/cram.h

clean:
	-rm -f *.o

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d cram


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

