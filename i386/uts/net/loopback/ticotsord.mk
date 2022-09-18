#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/loopback/ticotsord.mk	1.5"
#ident 	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
LOCALDEF = -DTICOTSORD
TCOOSED  = /bin/sed -e s/ticots/ticotsord/g -e s/tco/tcoo/g -e s/TCO/TCOO/g

TICOTSORD = $(CONF)/pack.d/ticotsor/Driver.o

CFILES = ticotsord.c

all: ID $(TICOTSORD)

ID:
	cd ticotsor.cf; $(IDINSTALL) -R$(CONF) -M ticotsor

$(TICOTSORD): ticotsord.o 
	$(LD) -r -o $@  ticotsord.o

ticotsord.c: ticots.c
	$(TCOOSED) <ticots.c >ticotsord.c

clean:
	-rm -f  ticotsord.o

clobber: clean
	$(IDINSTALL) -e -R$(CONF) -d ticotsor

headinstall: \
	$(KBASE)/net/loopback/ticotsord.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin $(KBASE)/net/loopback/ticotsord.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

