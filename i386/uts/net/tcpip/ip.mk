#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/tcpip/ip.mk	1.6"
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
LOCALDEF = -DSYSV -DSLIP
INSPERM  = -m 644 -u $(OWN) -g $(GRP)
IP = $(CONF)/pack.d/ip/Driver.o
ICMP = $(CONF)/pack.d/icmp/Driver.o
RAWIP = $(CONF)/pack.d/rawip/Driver.o

PRODUCTS = $(IP) $(ICMP) $(RAWIP)
OBJ = in.o in_cksum.o in_pcb.o in_switch.o in_transp.o ip_input.o \
	ip_output.o ip_main.o netlib.o route.o ip_vers.o

ROBJ=	raw_ip_main.o raw_ip.o raw_ip_cb.o

CFILES = $(OBJ:.o=.c) $(ROBJ:.o=.c) ip_icmp.c

CLEAN = ip.o ip_icmp.o icmp.o $(OBJ) $(ROBJ)


all : ID $(PRODUCTS)

ID:
	cd ip.cf; $(IDINSTALL) -R$(CONF) -M ip
	cd icmp.cf; $(IDINSTALL) -R$(CONF) -M icmp
	cd rawip.cf; $(IDINSTALL) -R$(CONF) -M rawip

$(IP):	$(OBJ)
	$(LD) -r -o $@ $(OBJ)

$(RAWIP):	$(ROBJ)
	$(LD) -r -o $@ $(ROBJ)
	
$(ICMP):	ip_icmp.o 
	-rm -f icmp.o
	ln ip_icmp.o icmp.o
	$(LD) -r -o $@ icmp.o
	rm icmp.o

clean:
		-rm -f $(CLEAN)

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d ip
	$(IDINSTALL) -e -R$(CONF) -d icmp
	$(IDINSTALL) -e -R$(CONF) -d rawip

headinstall: \
	$(KBASE)/net/tcpip/icmp_var.h \
	$(KBASE)/net/tcpip/if.h \
	$(KBASE)/net/tcpip/if_arp.h \
	$(KBASE)/net/tcpip/if_ether.h \
	$(KBASE)/net/tcpip/in.h \
	$(KBASE)/net/tcpip/in_pcb.h \
	$(KBASE)/net/tcpip/in_systm.h \
	$(KBASE)/net/tcpip/in_var.h \
	$(KBASE)/net/tcpip/insrem.h \
	$(KBASE)/net/tcpip/ip.h \
	$(KBASE)/net/tcpip/ip_icmp.h \
	$(KBASE)/net/tcpip/ip_str.h \
	$(KBASE)/net/tcpip/ip_var.h \
	$(FRC)
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/icmp_var.h
	$(INS) -f $(INC)/net     -m 644 $(INSPERM)  $(KBASE)/net/tcpip/if.h
	$(INS) -f $(INC)/net     -m 644 $(INSPERM)  $(KBASE)/net/tcpip/if_arp.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/if_ether.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/in.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/in_pcb.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/in_systm.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/in_var.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/insrem.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/ip.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/ip_icmp.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/ip_str.h
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM)  $(KBASE)/net/tcpip/ip_var.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

