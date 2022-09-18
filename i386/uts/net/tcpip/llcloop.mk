#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/tcpip/llcloop.mk	1.6"
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
LLCLOOP = $(CONF)/pack.d/llcloop/Driver.o
PRODUCTS = $(LLCLOOP)
CLEAN = llcloop.o

CFILES = llcloop.c

all : ID $(PRODUCTS)

ID:
	cd llcloop.cf; $(IDINSTALL) -R$(CONF) -M llcloop


$(LLCLOOP):	llcloop.o 
	$(LD) -r -o $@ llcloop.o

clean:
	-rm -f $(CLEAN)

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d llcloop

headinstall: \
	$(KBASE)/net/tcpip/llcloop.h \
	$(FRC)
	[ -d $(INC)/netinet ] || mkdir $(INC)/netinet
	$(INS) -f $(INC)/netinet -m 644 $(INSPERM) $(KBASE)/net/tcpip/llcloop.h

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

