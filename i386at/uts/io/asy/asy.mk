#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/asy/asy.mk	1.10"
#ident	"$Header: $"

include $(UTSRULES)

KBASE = ../..

GRP = bin
OWN = bin
HINSPERM = -m 644 -u $(OWN) -g $(GRP)

HEADERS = asy.h iasy.h asyhp.h asyc.h

IASY = $(CONF)/pack.d/iasy/Driver.o
ASYHP = $(CONF)/pack.d/asyhp/Driver.o
ASYC = $(CONF)/pack.d/asyc/Driver.o
 
CFILES = asyhp.c iasy.c asyc.c

all:	ID $(IASY) $(ASYHP) $(ASYC)

ID:	
	cd iasy.cf; $(IDINSTALL) -M -R$(CONF) iasy
	cd asyhp.cf; $(IDINSTALL) -M -R$(CONF) asyhp
	cd asyc.cf; $(IDINSTALL) -M -R$(CONF) asyc

headinstall:	
	@for i in $(HEADERS); \
	do \
		$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
	done

clean:	$(FRC)
	rm -f *.o

clobber:	clean
	$(IDINSTALL) -e -d -R$(CONF) iasy
	$(IDINSTALL) -e -d -R$(CONF) asyhp
	$(IDINSTALL) -e -d -R$(CONF) asyc

$(ASYHP):	asyhp.o
	$(LD) -r -o $@ asyhp.o

$(IASY):	iasy.o
	$(LD) -r -o $@ iasy.o

$(ASYC):	asyc.o
	$(LD) -r -o $@ asyc.o


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

