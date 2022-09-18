#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/mem/mem.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

KBASE     = ../..
MEM       = $(CONF)/pack.d/mm/Driver.o
DFILES    = mem.o

CFILES = $(DFILES:.o=.c)

all:	ID $(MEM)

ID:
	cd mm.cf; $(IDINSTALL) -R$(CONF) -M mm

$(MEM):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

clean:
	-rm -f $(DFILES)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d mm

headinstall:


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

