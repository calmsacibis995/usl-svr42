#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/loopback/ticlts.mk	1.6"
#ident 	"$Header: $"

include $(UTSRULES)

KBASE = ../..
TICLTS = $(CONF)/pack.d/ticlts/Driver.o

FILES = \
	ticlts.o

CFILES = $(FILES:.o=.c)


all: ID $(TICLTS)

ID:
	cd ticlts.cf; $(IDINSTALL) -R$(CONF) -M ticlts

$(TICLTS): ticlts.o 
	$(LD) -r -o $(TICLTS) $(FILES)

clean:
	-rm -f $(FILES)

clobber: clean
	$(IDINSTALL) -e -R$(CONF) -d ticlts

headinstall: \
	$(KBASE)/net/loopback/ticlts.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/net/loopback/ticlts.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

