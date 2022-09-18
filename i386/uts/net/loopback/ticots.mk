#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/loopback/ticots.mk	1.6"
#ident 	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
LOCALDEF = -DTICOTS
TICOTS = $(CONF)/pack.d/ticots/Driver.o

all: ID $(TICOTS)

CFILES = ticots.c

ID:
	cd ticots.cf; $(IDINSTALL) -R$(CONF) -M ticots

$(TICOTS): ticots.o 
	$(LD) -r -o $@ ticots.o

clean:
	-rm -f *.o

clobber: clean
	$(IDINSTALL) -e -R$(CONF) -d ticots

headinstall: \
	$(KBASE)/net/loopback/ticots.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/net/loopback/ticots.h

FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

