#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:util/weitek/weitek.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
CNFFILE  = $(CONF)/pack.d/weitek/Driver.o
DFILES   = weitek.o \
           ctl87.o	

CFILES1 = $(DFILES:.o=.c)

CFLAGS1 = $(CFLAGS)
DEFLIST1 = $(DEFLIST) -DWEITEK


all:	ID $(CNFFILE)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd weitek.cf; $(IDINSTALL) -R$(CONF) -M weitek


#
# Header Install Section
#
headinstall: \
	$(KBASE)/util/weitek.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/util/weitek.h

clean:
	-rm -f *.o

clobber:
	$(IDINSTALL) -e -R$(CONF) -d weitek


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

