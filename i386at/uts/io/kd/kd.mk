#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/kd/kd.mk	1.12"
#ident	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
CNFFILE  = $(CONF)/pack.d/kd/Driver.o
DFILES   = \
	btc.o	\
	evc.o	\
	kdv.o	\
	kdkb.o	\
	vdc.o	\
	i8042.o	\
	kbmode.o	\
	kd_cgi.o	\
	kdstr.o	\
	vtables.o	\
	kdvt.o	\
	kdvmstr.o	\
	evga.o

CFILES = $(DFILES:.o=.c)


all:	ID $(CNFFILE)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

#
# Configuration Section
#
ID:
	cd kd.cf; $(IDINSTALL) -R$(CONF) -M kd

#
# Header Install Section
#
headinstall: \
	$(KBASE)/io/kd/kb.h \
	$(KBASE)/io/kd/kd_btc.h \
	$(KBASE)/io/kd/btc.h \
	$(KBASE)/io/kd/kd.h \
	$(KBASE)/io/kd/kd_cgi.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/kd/kb.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/kd/kd.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/kd/btc.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/kd/kd_btc.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/kd/kd_cgi.h

clean:
	-rm -f *.o

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d kd


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

