#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:proc/nfa/nfa.mk	1.2"
#ident	"$Header: $"

include $(UTSRULES)

KBASE = ../..
all:	ID

ID:
	cd nfa.cf; $(IDINSTALL) -R$(CONF) -M nfa

clean:
	-rm -f *.o

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d nfa

headinstall:

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

