#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Copyrighted as an unpublished work.
#	(c) Copyright 1989 INTERACTIVE Systems Corporation
#	All rights reserved.

#	RESTRICTED RIGHTS

#	These programs are supplied under a license.  They may be used,
#	disclosed, and/or copied only as permitted under such license
#	agreement.  Any copy must contain the above copyright notice and
#	this restricted rights notice.  Use, copying, and/or disclosure
#	of the programs is strictly prohibited unless otherwise provided
#	in the license agreement.

#ident	"@(#)uts-x86at:boot/at386/mip/mip.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

KBASE	  = ../../..

INSDIR = $(ROOT)/$(MACH)/etc/initprog

OBJS = mip.o olivetti.o compaq.o at386.o dell.o \
	mc386.o apricot.o intel.o necpm.o misc386.o

CFILES = $(OBJS:.o=.c)

all:	mip	

mip: $(OBJS) ipifile
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		${LD} -Mipifile -dn -e mip_start -o mip $(OBJS) ; \
	else \
		${LD} -e mip_start -o mip ipifile $(OBJS) ; \
	fi 

ipifile: mipifile ../sip_pconf.h
	cp mipifile ipifile.c
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		fgrep V0x ../sip_pconf.h | \
		awk '{ printf( "define(%s, %s)\n", $$2, $$3 )}' >mipifile.m4 ; \
		${M4} -D${CCSTYPE} ipifile.c >ipifile; \
	else \
		${CC} -P -D${CCSTYPE} ipifile.c ; \
		mv ipifile.i ipifile ; \
	fi 
	rm -f ipifile.c

clean:
	-/bin/rm -f $(OBJS) ipifile *.m4

clobber: clean
	-/bin/rm -f mip

install: all
	-[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 644 -u bin -g bin mip

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

