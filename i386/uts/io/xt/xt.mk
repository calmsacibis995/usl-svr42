#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/xt/xt.mk	1.5"
#ident "$Header: xt.mk 1.2 91/03/20 $"

include $(UTSRULES)

KBASE    = ../..
INSPERM  = -m 644 -u $(OWN) -g $(GRP)
CNFFILE  = $(CONF)/pack.d/xt/Driver.o
DFILES   = xt.o

CFILES = $(DFILES:.o=.c)



all:	ID $(CNFFILE)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd xt.cf; $(IDINSTALL) -R$(CONF) -M xt    

clean:
	-rm -f $(DFILES)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d xt

headinstall: \
	$(KBASE)/io/xt/jioctl.h \
	$(KBASE)/io/xt/nxt.h \
	$(KBASE)/io/xt/nxtproto.h \
	$(FRC)
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/io/xt/jioctl.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/io/xt/nxt.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/io/xt/nxtproto.h
	$(INS) -f $(INC)/sys $(INSPERM) $(KBASE)/io/xt/nxtproto.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

