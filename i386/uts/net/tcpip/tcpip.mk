#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/tcpip/tcpip.mk	1.5"
#ident 	"$Header: $"

include $(UTSRULES)

KBASE    = ../..
LOCALDEF = -DSYSV
INSPERM  = -m 644 -u $(OWN) -g $(GRP)
PRODUCTS = arp ip llcloop tcp udp


all:
	@for i in $(PRODUCTS) ; \
	do \
		echo "====== $(MAKE) -f $$i.mk all";\
		$(MAKE) -f $$i.mk all $(MAKEARGS) ;\
	done

depend:: makedep
	@for i in $(PRODUCTS) ; \
	do \
		echo "====== $(MAKE) -f $$i.mk depend";\
		$(MAKE) -f $$i.mk depend MAKEFILE=$$i.mk $(MAKEARGS); \
	done

clean:	
	@for  i in $(PRODUCTS) ;\
	do\
		echo "====== $(MAKE) -f $$i.mk clean ";\
		$(MAKE) -f $$i.mk clean $(MAKEARGS) ;\
	done

clobber:	
	@for  i in $(PRODUCTS) ;\
	do\
		echo "====== $(MAKE) -f $$i.mk clobber ";\
		$(MAKE) -f $$i.mk clobber $(MAKEARGS) ;\
	done

headinstall: \
	$(KBASE)/net/tcpip/byteorder.h \
	$(KBASE)/net/tcpip/lihdr.h \
	$(KBASE)/net/tcpip/protosw.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 $(INSPERM) $(KBASE)/net/tcpip/byteorder.h
	$(INS) -f $(INC)/sys -m 644 $(INSPERM) $(KBASE)/net/tcpip/lihdr.h
	$(INS) -f $(INC)/sys -m 644 $(INSPERM) $(KBASE)/net/tcpip/protosw.h
	@for i in $(PRODUCTS) ; \
	do \
		$(MAKE) -f $$i.mk headinstall $(MAKEARGS) ;\
	done

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

