#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/ansi/ansi.mk	1.8"
#ident	"$Header: $"

include $(UTSRULES)

KBASE   = ../..
CNFFILE = $(CONF)/pack.d/ansi/Driver.o
DFILES  = ansi.o

CFILES = $(DFILES:.o=.c)


all:	ID $(CNFFILE)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd ansi.cf; $(IDINSTALL) -R$(CONF) -M ansi

#
# Header Install Section
#
headinstall: \
	$(KBASE)/io/ansi/ansi.h \
	$(KBASE)/io/ansi/at_ansi.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/ansi/ansi.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/ansi/at_ansi.h
	
clean:
	-rm -f $(DFILES)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d ansi



FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

