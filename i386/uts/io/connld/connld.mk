#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/connld/connld.mk	1.5"
#ident	"$Header: $"
#ident "$Header: connld.mk 1.2 91/03/20 $"

include $(UTSRULES)

KBASE    = ../..
CNFFILE  = $(CONF)/pack.d/connld/Driver.o
DFILES   = connld.o

CFILES = $(DFILES:.o=.c)



all:	ID $(CNFFILE)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd connld.cf; $(IDINSTALL) -R$(CONF) -M connld

#
# Cleanup Section
#
clean:
	-rm -f $(DFILES)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d connld

headinstall:


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

