#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/rmc/rmc.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
CNFFILE  = $(CONF)/pack.d/rmc/Driver.o
DFILES   = rmc.o

all:	ID

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd rmc.cf; $(IDINSTALL) -R$(CONF) -M rmc


#
# Header Install Section
#
headinstall:

clean:
	-rm -rf rmc.o

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d rmc

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

