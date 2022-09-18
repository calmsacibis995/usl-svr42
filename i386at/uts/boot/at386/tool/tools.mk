#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)uts-x86at:boot/at386/tool/tools.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

KBASE	= ../../..
LIBELF = $(TOOLS)/usr/ccs/lib/libelf$(PFX).a

all:  tdxtract progconf

install:	all

tdxtract:	tdxtract.c hdrs
	$(HCC) $(CFLAGS) -Iinc $(GLOBALINC) -o tdxtract tdxtract.c $(LIBELF)

progconf:	progconf.c hdrs
	$(HCC) $(CFLAGS) -Iinc $(GLOBALINC) -o progconf progconf.c $(LIBELF)

hdrs:
	[ -d inc ] || mkdir inc
	[ -d inc/sys ] || mkdir inc/sys
	cp $(TOOLS)/usr/include/libelf.h ./inc
	cp $(TOOLS)/usr/include/sys/elf.h ./inc/sys
	cp $(TOOLS)/usr/include/sys/elftypes.h ./inc/sys
	chmod 0644 inc/*.h inc/sys/*.h

clean:
	-/bin/rm -f tdxtract.o progconf.o

clobber: clean
	-/bin/rm -fr tdxtract progconf inc

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

