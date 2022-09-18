#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/osxt/osxt.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

KBASE      = ../..
OSXT       = $(CONF)/pack.d/osxt/Driver.o
FILES      = osxt.o

CFILES = $(FILES:.o=.c)

all:	ID $(OSXT)

ID:
	cd osxt.cf; $(IDINSTALL) -R$(CONF) -M osxt

$(OSXT): $(FILES)
	$(LD) -r -o $@ $(FILES)

clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d osxt

headinstall: \
	$(KBASE)/io/osxt/sxt.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u bin -g bin  $(KBASE)/io/osxt/sxt.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

