#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:net/des/des.mk	1.6"
#ident 	"$Header: $"

#
#
#
#  		PROPRIETARY NOTICE (Combined)
#  
#  This source code is unpublished proprietary information
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
#	Kernel DES
#
include $(UTSRULES)

KBASE = ../..
LOCALDEF = -DSYSV
OFILE = $(CONF)/pack.d/des/Driver.o
DFILE = $(CONF)/pack.d/des/Driver_d.o
IFILE = $(CONF)/pack.d/des/Driver_i.o

DESOBJ =  des_crypt.o des_soft.o

IDESOBJ =  intldescrypt.o intldes_soft.o

CFILES = $(DESOBJ:.o=.c) $(IDESOBJ:.o=.c)

ALL:	ID
		if [ -s des_crypt.c -a  -s des_soft.c ] ;\
		then \
			$(MAKE) -f des.mk domestic $(MAKEARGS) ;\
		fi
		$(MAKE) -f des.mk intl $(MAKEARGS); \
		rm -f $(OFILE); ln $(IFILE) $(OFILE)

ID:
	cd des.cf; $(IDINSTALL) -R$(CONF) -M des

lint:
	lint $(CFLAGS) -Dlint *.c

domestic:	$(DESOBJ)
	$(LD) -r -o $(DFILE) $(DESOBJ)

intl:	$(IDESOBJ)
	$(LD) -r -o $(IFILE) $(IDESOBJ)

headinstall: \
	$(KBASE)/net/des/des.h \
	$(KBASE)/net/des/desdata.h \
	$(KBASE)/net/des/softdes.h \
	$(FRC)
	$(INS) -f $(INC)/des -m 644 -u $(OWN) -g $(GRP) $(KBASE)/net/des/des.h
	$(INS) -f $(INC)/des -m 644 -u $(OWN) -g $(GRP) $(KBASE)/net/des/desdata.h
	$(INS) -f $(INC)/des -m 644 -u $(OWN) -g $(GRP) $(KBASE)/net/des/softdes.h

clean:
	rm -f *.o

clobber:	clean
		$(IDINSTALL) -e -R$(CONF) -d des

FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

