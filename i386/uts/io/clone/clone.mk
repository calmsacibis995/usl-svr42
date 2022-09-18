#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/clone/clone.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
CLONE    = $(CONF)/pack.d/clone/Driver.o
DFILES	 = clone.o

CFILES = $(DFILES:.o=.c)

all:	ID $(CLONE)

ID:
	cd clone.cf; $(IDINSTALL) -R$(CONF) -M clone

$(CLONE):	$(DFILES)
	$(LD) -r -o $@ clone.o

headinstall:
	$(FRC)

clean:
	-rm -f $(DFILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d clone


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

