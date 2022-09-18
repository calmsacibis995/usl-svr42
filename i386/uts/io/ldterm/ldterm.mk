#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/ldterm/ldterm.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
LOCALDEF = -DDDI_OFF
INSPERM  = -m 644 -u $(OWN) -g $(GRP)
CNFFILE  = $(CONF)/pack.d/ldterm/Driver.o
DFILES   = ldterm.o 

CFILES = $(DFILES:.o=.c)

all:	ID $(CNFFILE)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd ldterm.cf; $(IDINSTALL) -R$(CONF) -M ldterm

clean:
	-rm -f $(DFILES)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d ldterm

headinstall: \
	$(KBASE)/io/ldterm/euc.h \
	$(KBASE)/io/ldterm/eucioctl.h \
	$(KBASE)/io/ldterm/ldterm.h \
	$(FRC)
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/io/ldterm/euc.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/io/ldterm/eucioctl.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/io/ldterm/ldterm.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

