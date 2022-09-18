#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:acc/dac/dac.mk	1.5"
#ident	"$Header: $"
#ident "$Header: dac.mk 1.1 91/03/21 $"

include $(UTSRULES)

KBASE    = ../..
DAC      = $(CONF)/pack.d/dac/Driver.o

FILES =\
	vndac.o \
	gendac.o \
	ipcdac.o 

CFILES = $(FILES:.o=.c)

all:	ID $(DAC)

$(DAC): $(FILES)
	$(LD) -r -o $@ $(FILES)


#
# Configuration Section
#
ID:
	cd dac.cf; $(IDINSTALL) -R$(CONF) -M dac

clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d dac  


headinstall: \
	$(KBASE)/acc/dac/acl.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/acc/dac/acl.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

