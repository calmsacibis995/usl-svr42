#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/lockmgr/lockmgr.mk	1.4"
#ident "$Header: proc.mk 1.5 91/03/28 $"

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

KBASE =	../..
LOCALDEF = -DSYSV
KLM = $(CONF)/pack.d/klm/Driver.o

FILES = \
	klm_kprot.o \
	klm_lkmgr.o

CFILES = $(FILES:.o=.c)



all:	ID $(KLM)

ID:
	cd klm.cf; $(IDINSTALL) -R$(CONF) -M klm

$(KLM):	$(FILES)
	$(LD) -r -o $@ $(FILES)

headinstall:	\
	$(KBASE)/net/lockmgr/klm_prot.h \
	$(KBASE)/net/lockmgr/lockmgr.h \
	$(FRC)
	$(INS) -f $(INC)/klm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/net/lockmgr/klm_prot.h
	$(INS) -f $(INC)/klm -m 644 -u $(OWN) -g $(GRP) $(KBASE)/net/lockmgr/lockmgr.h

clean:
	-rm -f *.o

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d klm


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

