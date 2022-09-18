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

#ident	"@(#)uts-x86at:boot/at386/sip/sip.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

KBASE	  = ../../..

ASFLAGS = -m
INSDIR = $(ROOT)/$(MACH)/etc/initprog

FILES = sip 
OBJFS = sip.o memsizer.o brgmgr.o kswitch.o 

CFILES =  \
	sip.c \
	memsizer.c \
	brgmgr.c

all:	$(FILES)

sip: $(OBJFS) ipifile
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		${LD} -Mipifile -dn -e sip_start -o sip $(OBJFS) ; \
	else \
		${LD} -e sip_start -o sip ipifile $(OBJFS) ; \
	fi 
	../tool/progconf sip ../sip_pconf.h

ipifile: sipifile ../wsbt_pconf.h
	cp sipifile ipifile.c
	if [ x${CCSTYPE} = xELF ] ; \
	then \
		fgrep V0x ../wsbt_pconf.h | \
		awk '{printf( "define(%s, %s)\n", $$2, $$3 )}' >sipifile.m4 ; \
		${M4} -D${CCSTYPE} ipifile.c >ipifile ; \
	else \
		${CC} -P -D${CCSTYPE} ipifile.c ; \
		mv ipifile.i ipifile ; \
	fi 
	rm -f ipifile.c

kswitch.o: kswitch.s kswitch.m4
	-/bin/rm -f kswitch.i
	$(AS) $(ASFLAGS) kswitch.s

kswitch.m4:	kswitch_sym.c $(KBASE)/boot/bootdef.h
	$(CC) $(CFLAGS) $(DEFLIST) -S kswitch_sym.c
	$(AWK) -f ../syms.awk < kswitch_sym.s | \
	$(SED) -e '1,$$s;__SYMBOL__;;' >kswitch.m4

clean:
	-/bin/rm -f $(OBJFS) ipifile kswitch.i *.m4 *_sym.s

clobber: clean
	-/bin/rm -f $(FILES) ../sip_pconf.h

install: all
	-[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 644 -u bin -g bin sip

FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

