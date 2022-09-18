#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/tcpip/udp.mk	1.6"
#ident 	"$Header: $"
#
#  		PROPRIETARY NOTICE (Combined)
#  
#  This source code is unpublished proprietary information
#  constituting, or derived under license from AT&T's Unix(r) System V.
#  In addition, portions of such source code were derived from Berkeley
#  4.3 BSD under license from the Regents of the University of
#  California.
#  
#  
#  
#  		Copyright Notice 
#  
#  Notice of copyright on this source code product does not indicate 
#  publication.
#  
#  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
#  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#  	          All rights reserved.
#
include $(UTSRULES)

KBASE    = ../..
LOCALDEF = -DSYSV
INSPERM  = -m 644 -u $(OWN) -g $(GRP)

UDP = $(CONF)/pack.d/udp/Driver.o
PRODUCTS = $(UDP)
OBJ = udp_io.o udp_main.o udp_state.o 
CLEAN = udp.o $(OBJ)

CFILES = $(OBJ:.o=.c)

all : ID $(PRODUCTS)

ID:
	cd udp.cf; $(IDINSTALL) -R$(CONF) -M udp

$(UDP): $(OBJ)
	$(LD) -r -o $@ $(OBJ)

clean:
		-rm -f $(CLEAN)

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d udp

headinstall: \
	$(KBASE)/net/tcpip/udp.h \
	$(KBASE)/net/tcpip/udp_var.h \
	$(FRC)
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM) $(KBASE)/net/tcpip/udp.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM) $(KBASE)/net/tcpip/udp_var.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

