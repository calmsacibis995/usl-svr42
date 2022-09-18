#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/ktli/ktli.mk	1.6"
#ident "$Header: proc.mk 1.5 91/03/28 $"

#	ktli.mk 1.2 89/01/11 SMI
#
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
#
#	Kernel TLI inteface
#
include $(UTSRULES)

KBASE = ../..
LOCALDEF = -DSYSV
KTLI = $(CONF)/pack.d/ktli/Driver.o
KTLIOBJ = t_kclose.o t_kgtstate.o t_ksndudat.o t_kutil.o t_kalloc.o \
	  t_kconnect.o t_kopen.o t_kspoll.o t_kbind.o t_kunbind.o t_kfree.o \
	  t_krcvudat.o 

CFILES = $(KTLIOBJ:.o=.c)



all:	ID $(KTLI)

ID:
	cd ktli.cf; $(IDINSTALL) -R$(CONF) -M ktli

$(KTLI):	$(KTLIOBJ)
	$(LD) -r -o $@ $(KTLIOBJ)

headinstall:	\
	$(KBASE)/net/ktli/t_kuser.h \
	$(KBASE)/net/transport/tihdr.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/net/ktli/t_kuser.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/net/transport/tihdr.h
lint:
	lint $(CFLAGS) -Dlint *.c

clean:
	rm -f *.o

clobber:	clean
	$(IDINSTALL) -e -R$(CONF) -d ktli


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

