#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uts-x86:acc/mac/mac.mk	1.6"
#ident	"$Header: $"
#ident "$Header: mac.mk 1.2 91/03/21 $"

include $(UTSRULES)

KBASE    = ../..
MAC      = $(CONF)/pack.d/mac/Driver.o

FILES =\
	genmac.o \
	procmac.o \
	vnmac.o \
	covert.o \
	ipcmac.o

CFILES = $(FILES:.o=.c)


all:	ID $(MAC)

$(MAC): $(FILES)
	$(LD) -r -o $@ $(FILES)


#
# Configuration Section
#
ID:
	cd mac.cf; $(IDINSTALL) -R$(CONF) -M mac

clean:
	-rm -f $(FILES)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d mac  



headinstall: \
	$(KBASE)/acc/mac/covert.h \
	$(KBASE)/acc/mac/mac.h \
	$(FRC)
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/acc/mac/covert.h
	$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $(KBASE)/acc/mac/mac.h



FRC:

include $(UTSDEPEND)
#
#	Header dependencies
#

# DO NOT DELETE THIS LINE (make depend uses it)

