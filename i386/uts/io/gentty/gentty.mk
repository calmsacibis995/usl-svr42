#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/gentty/gentty.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

KBASE     = ../..
GEN       = $(CONF)/pack.d/gentty/Driver.o
FILES     = gentty.o

CFILES = $(FILES:.o=.c)

all:	ID $(GEN)

ID:
	cd gentty.cf; $(IDINSTALL) -R$(CONF) -M gentty

$(GEN):	$(FILES)
	$(LD) -r -o $(GEN) gentty.o

clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d gentty

headinstall:


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

