#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)makekey:makekey.mk	1.6.7.2"
#ident  "$Header: makekey.mk 1.3 91/06/28 $"

include $(CMDRULES)

#	Makefile for makekey

OWN = bin
GRP = bin

LDLIBS = -lcrypt_i

all: makekey

makekey: makekey.o 
	$(CC) -o makekey makekey.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

makekey.o: makekey.c

install: all
	 $(INS) -f $(USRLIB) -m 0555 -u $(OWN) -g $(GRP) makekey

clean:
	rm -f makekey.o

clobber: clean
	rm -f makekey

lintit:
	$(LINT) $(LINTFLAGS) makekey.c

#	These targets are useful but optional

partslist:
	@echo makekey.mk makekey.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRLIB) | tr ' ' '\012' | sort

product:
	@echo makekey | tr ' ' '\012' | \
	sed 's;^;$(USRLIB)/;'

srcaudit:
	@fileaudit makekey.mk $(LOCALINCS) makekey.c -o makekey.o makekey
