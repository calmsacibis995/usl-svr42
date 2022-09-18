#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/iaf/iaf.mk	1.4"
#ident	"$Header: $"
#ident "$Header: iaf.mk 1.2 91/03/20 $"

# 
# Identification and Authentication STREAMS Module
#


include $(UTSRULES)

KBASE    = ../..
INSPERM  = -m 644 -u $(OWN) -g $(GRP)
CNFFILE  = $(CONF)/pack.d/iaf/Driver.o
DFILES   = iaf.o

CFILES = $(DFILES:.o=.c)



all:	ID $(CNFFILE)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd iaf.cf; $(IDINSTALL) -R$(CONF) -M iaf

headinstall: \
	$(KBASE)/io/iaf/iaf.h \
	$(FRC)
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/io/iaf/iaf.h

clean:
	-rm -f $(DFILES)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d iaf  


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

