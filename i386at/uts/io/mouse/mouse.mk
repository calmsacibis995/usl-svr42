#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86at:io/mouse/mouse.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

KBASE = ../..

MSE = $(CONF)/pack.d/mse/Driver.o
BMSE = $(CONF)/pack.d/bmse/Driver.o
SMSE = $(CONF)/pack.d/smse/Driver.o
M320 = $(CONF)/pack.d/m320/Driver.o

all:	ID $(MSE) $(SMSE) $(BMSE) $(M320)

ID:
	cd mse.cf ; $(IDINSTALL) -M -R$(CONF) mse
	cd bmse.cf ; $(IDINSTALL) -M -R$(CONF) bmse
	cd smse.cf ; $(IDINSTALL) -M -R$(CONF) smse
	cd m320.cf ; $(IDINSTALL) -M -R$(CONF) m320

clean: $(FRC)
	rm -f mouse.o smse.o bmse.o m320.o mse_subr.o

clobber: clean
	$(IDINSTALL) -e -d -R$(CONF) mse
	$(IDINSTALL) -e -d -R$(CONF) bmse
	$(IDINSTALL) -e -d -R$(CONF) smse
	$(IDINSTALL) -e -d -R$(CONF) m320

$(MSE):	mouse.o mse_subr.o
	$(LD) -r -o $(MSE) mouse.o mse_subr.o

$(M320):	m320.o 
	$(LD) -r -o $(M320) m320.o

$(BMSE):	bmse.o 
	$(LD) -r -o $(BMSE) bmse.o

$(SMSE):	smse.o 
	$(LD) -r -o $(SMSE) smse.o

mouse.o:	mouse.c mse.h $(FRC)

smse.o:	smse.c mse.h $(FRC)

bmse.o:	bmse.c mse.h $(FRC)

m320.o:	m320.c mse.h $(FRC)

mse_subr.o:	mse_subr.c mse.h $(FRC)

#
# Header Install Section
#
headinstall: \
	$(KBASE)/io/mouse/mse.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/mouse/mse.h

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

