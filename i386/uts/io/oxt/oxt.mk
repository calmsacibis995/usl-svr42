#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:io/oxt/oxt.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

KBASE      = ../..
OXT        = $(CONF)/pack.d/oxt/Driver.o
FILES      = oxt.o

CFILES = $(FILES:.o=.c)

all:	ID $(OXT)

ID:
	cd oxt.cf; $(IDINSTALL) -R$(CONF) -M oxt

$(OXT): $(FILES)
	$(LD) -r -o $@ $(FILES)

clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d oxt

headinstall: \
	$(KBASE)/io/oxt/xt.h \
	$(KBASE)/io/oxt/xtproto.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/oxt/xt.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP)  $(KBASE)/io/oxt/xtproto.h


FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

