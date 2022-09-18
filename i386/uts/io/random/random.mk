#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/random/random.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

KBASE     = ../..
RAND      = $(CONF)/pack.d/rand/Driver.o
FILES     = random.o

CFILES = $(FILES:.o=.c)

ID:
	cd random.cf; $(IDINSTALL) -R$(CONF) -M rand

all:	ID $(RAND)

$(RAND):	$(FILES)
	$(LD) -r -o $@ $(FILES)

clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d rand

headinstall:


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

