#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/tcpip/arp.mk	1.6"
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

KBASE = ../..
LOCALDEF = -DSYSV
ARP = $(CONF)/pack.d/arp/Driver.o
APP = $(CONF)/pack.d/app/Driver.o
PRODUCTS = $(ARP) $(APP)

CFILES = app.c arp.c

all: ID $(PRODUCTS)

ID:
	cd app.cf; $(IDINSTALL) -R$(CONF) -M app
	cd arp.cf; $(IDINSTALL) -R$(CONF) -M arp

$(ARP):	arp.o 
	$(LD) -r -o $@ arp.o

$(APP):	app.o 
	$(LD) -r -o $@ app.o

clean:
	-rm -f *.o

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d app
	$(IDINSTALL) -e -R$(CONF) -d arp

headinstall: \
	$(KBASE)/net/tcpip/af.h \
	$(KBASE)/net/tcpip/arp.h \
	$(FRC)
	$(INS) -f $(INC)/net -m 644 -u $(OWN) -g $(GRP) $(KBASE)/net/tcpip/af.h
	$(INS) -f $(INC)/net -m 644 -u $(OWN) -g $(GRP) $(KBASE)/net/tcpip/arp.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

