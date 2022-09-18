#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/pt/pt.mk	1.5"
#ident "$Header: pt.mk 1.2 91/03/20 $"

include $(UTSRULES)

KBASE    = ../..
LOCALDEF = -UDBUG
PTMFILE  = $(CONF)/pack.d/ptm/Driver.o
PTSFILE  = $(CONF)/pack.d/pts/Driver.o
PTEMFILE = $(CONF)/pack.d/ptem/Driver.o
PCKTFILE = $(CONF)/pack.d/pckt/Driver.o
DFILES   = ptm.o pts.o ptem.o pckt.o

CFILES = $(DFILES:.o=.c)

all: 	ID $(PTMFILE) $(PTSFILE) $(PTEMFILE) $(PCKTFILE)

$(PTMFILE):	ptm.o
	$(LD) -r -o $@ ptm.o

$(PTSFILE):	pts.o
	$(LD) -r -o $@ pts.o

$(PTEMFILE):	ptem.o
	$(LD) -r -o $@ ptem.o

$(PCKTFILE):	pckt.o
	$(LD) -r -o $@ pckt.o

#
# Configuration Section
#
ID:
	cd ptm.cf; $(IDINSTALL) -R$(CONF) -M ptm   
	cd pts.cf; $(IDINSTALL) -R$(CONF) -M pts   
	cd ptem.cf; $(IDINSTALL) -R$(CONF) -M ptem  
	cd pckt.cf; $(IDINSTALL) -R$(CONF) -M pckt  

clean:
	-rm -f $(DFILES)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d ptm
	-$(IDINSTALL) -R$(CONF) -e -d pts
	-$(IDINSTALL) -R$(CONF) -e -d ptem
	-$(IDINSTALL) -R$(CONF) -e -d pckt

headinstall: \
	$(KBASE)/io/pt/ptem.h \
	$(KBASE)/io/pt/ptms.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/io/pt/ptem.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/io/pt/ptms.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

