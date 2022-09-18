#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/fd/fd.mk	1.11"
#ident	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
CNFFILE  = $(CONF)/pack.d/fd/Driver.o
DFILES   = fd.o

CNFFILE2  = $(CONF)/pack.d/fdbuf/Driver.o
DFILES2   = fdbuf.o

CFILES = $(DFILES:.o=.c) $(DFILES2:.o=.c)

all:	ID $(CNFFILE) $(CNFFILE2)

$(CNFFILE):	$(DFILES)
	$(LD) -r -o $@ $(DFILES)

$(CNFFILE2):	$(DFILES2)
	$(LD) -r -o $@ $(DFILES2)

#
# Configuration Section
#
ID:
	cd fd.cf; $(IDINSTALL) -R$(CONF) -M fd
	cd fdbuf.cf; $(IDINSTALL) -R$(CONF) -M fdbuf


clean:
	-rm -f $(DFILES) $(DFILES2)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d fd
	-$(IDINSTALL) -R$(CONF) -e -d fdbuf

#
# Header Install Section
#
headinstall: \
	$(KBASE)/io/fd/fd.h \
	$(KBASE)/io/fd/sema.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/fd/fd.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/fd/sema.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

