#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/sysmsg/sysmsg.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
CNFFILE  = $(CONF)/pack.d/sysmsg/Driver.o
DFILES   = sysmsg.o

CFILES = $(DFILES:.o=.c)


all:	ID $(CNFFILE)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd sysmsg.cf; $(IDINSTALL) -R$(CONF) -M sysmsg


#
# Header Install Section
#
headinstall:	\
	$(KBASE)/io/sysmsg/sysmsg.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/sysmsg/sysmsg.h

clean:
	-rm -f *.o

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d sysmsg


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

