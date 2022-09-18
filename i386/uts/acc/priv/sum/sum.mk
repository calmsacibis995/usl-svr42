#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:acc/priv/sum/sum.mk	1.6"
#ident	"$Header: $"
#ident "$Header: sum.mk 1.1 91/03/21 $"

include $(UTSRULES)

KBASE    = ../../..
SUM      = $(CONF)/pack.d/sum/Driver.o
FILES    = sum.o

CFILES = $(FILES:.o=.c)

all:	ID $(SUM)

$(SUM): $(FILES)
	$(LD) -r -o $@ $(FILES)

#
# Configuration Section
#
ID:
	cd sum.cf; $(IDINSTALL) -R$(CONF) -M sum

clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d sum  


headinstall:


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

