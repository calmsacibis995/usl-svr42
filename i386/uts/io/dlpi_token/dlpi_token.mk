#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)uts-x86:io/dlpi_token/dlpi_token.mk	1.4"
#ident	"$Header: $"

#	Copyright (c) 1990 Dell Computer Corporation
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Dell Computer Corporation
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

include $(UTSRULES)

LOCALDEF	= -DSYSV -DSYSV4 -DINTEL_BYTE_ORDER -DVIANET -DM_INTR \
		  -DTOKDEBUG -DOBJECT_MARKING -DB_ASM -DLAI_TCP

KBASE		= ../..
IBMTOK		= $(CONF)/pack.d/ibmtok/Driver.o
IBMTOKFILES	= ibmtok.o

CFILES = $(IBMTOKFILES:.o=.c)

all:		ID $(IBMTOK)

$(IBMTOK):	$(IBMTOKFILES)
	$(LD) -r -o $@ $(IBMTOKFILES)

ID:
	cd ibmtok.cf; $(IDINSTALL) -R$(CONF) -M ibmtok

headinstall: \
	$(KBASE)/io/dlpi_token/ibmtok.h \
	$(KBASE)/io/dlpi_token/ibmtokhw.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)	 $(KBASE)/io/dlpi_token/ibmtok.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)	 $(KBASE)/io/dlpi_token/ibmtokhw.h


clean:
	-rm -f *.o

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d ibmtok

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

