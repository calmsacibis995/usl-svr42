#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)eac:i386/eaccmd/awk/awk.mk	1.4"
#ident  "$Header: awk.mk 1.1 91/08/12 $"

include $(CMDRULES)

USREAC=$(USR)/usr/eac

all clean clobber lintit:  

install: all
	- [ -d $(USREAC)/bin ] || mkdir -p $(USREAC)/bin
	rm -f $(USREAC)/bin/awk
	$(SYMLINK) /usr/bin/nawk $(USREAC)/bin/awk
