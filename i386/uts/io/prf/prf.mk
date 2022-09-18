#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/prf/prf.mk	1.5"
#ident "$Header: $"

include $(UTSRULES)

KBASE    = ../..
CNFFILE  = $(CONF)/pack.d/prf/Driver.o
DFILES   = prf.o modprf.o

CFILES = $(DFILES:.o=.c)

all:	ID $(CNFFILE)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd prf.cf; $(IDINSTALL) -R$(CONF) -M prf   

clean:
	-rm -f $(DFILES)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d prf

headinstall:	$(KBASE)/io/prf/prf.h \
		$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/io/prf/prf.h

FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

