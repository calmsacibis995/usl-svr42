#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/intmap/intmap.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
CNFFILE  = $(CONF)/pack.d/intmap/Driver.o
OFILES   = \
	nmap.o \
	emap.o \
	xmap.o

CFILES = $(OFILES:.o=.c)


all:	ID $(CNFFILE)

$(CNFFILE):	$(OFILES)
	$(LD) -r -o $@ $(OFILES)

#
# Configuration Section
#
ID:
	cd intmap.cf; $(IDINSTALL) -R$(CONF) -M intmap  


#
# Cleanup Section
#
clean:
	-rm -f $(OFILES)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d intmap


lint:


headinstall: \
	$(KBASE)/io/intmap/emap.h \
	$(KBASE)/io/intmap/nmap.h \
	$(KBASE)/io/intmap/xmap.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/intmap/emap.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/intmap/nmap.h
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/intmap/xmap.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

