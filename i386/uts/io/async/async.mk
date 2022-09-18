#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/async/async.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

KBASE     = ../..
all:	ID

ID:
	cd async.cf; $(IDINSTALL) -R$(CONF) -M async


headinstall:

clean:

clobber:	
	-$(IDINSTALL) -e -R$(CONF) -d async

FRC: 
 
include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

