#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/cpyrt/cpyrt.mk	1.3"
#ident "$Header: cpyrt.mk 1.4 91/03/21 $"

include $(UTSRULES)

KBASE    = ../..
CPYRT	 = $(CONF)/pack.d/cpyrt/Driver.o
DFILES   = cpyrt.o

CFILES = $(DFILES:.o=.c)



all:	ID $(CPYRT)

$(CPYRT):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd cpyrt.cf; $(IDINSTALL) -R$(CONF) -M cpyrt 

headinstall:

#
# Cleanup Section
#
clean:
	-rm -f $(DFILES)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d cpyrt



FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

