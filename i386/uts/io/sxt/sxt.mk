#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/sxt/sxt.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

KBASE     = ../..
SXT       = $(CONF)/pack.d/sxt/Driver.o
DFILES    = sxt.o

CFILES = $(DFILES:.o=.c)

all:	ID $(SXT)

ID:
	cd sxt.cf; $(IDINSTALL) -R$(CONF) -M sxt

$(SXT):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

clean:
	-rm -f $(DFILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d sxt

headinstall: \
	$(KBASE)/io/sxt/nsxt.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/sxt/nsxt.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

