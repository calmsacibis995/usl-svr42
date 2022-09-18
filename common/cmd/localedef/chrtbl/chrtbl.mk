#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)localedef:common/cmd/localedef/chrtbl/chrtbl.mk	1.1.7.3"
#ident "$Header: chrtbl.mk 1.2 91/04/17 $"

include $(CMDRULES)

#	Makefile for chrtbl

OWN = bin
GRP = bin

all: chrtbl

chrtbl: chrtbl.o 
	$(CC) -o chrtbl chrtbl.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

chrtbl.o: chrtbl.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/varargs.h \
	$(INC)/string.h \
	$(INC)/signal.h $(INC)/sys/signal.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) chrtbl
	$(INS) -f $(USRLIB)/locale/C -m 0555 -u $(OWN) -g $(GRP) chrtbl_C
	$(CH)./chrtbl chrtbl_C
	$(CH)$(INS) -f $(USRLIB)/locale/C LC_CTYPE
	$(CH)$(INS) -f $(USRLIB)/locale/C LC_NUMERIC
	$(CH)rm -f LC_CTYPE LC_NUMERIC

clean:
	rm -f chrtbl.o ctype.c

clobber: clean
	rm -f chrtbl

lintit:
	$(LINT) $(LINTFLAGS) chrtbl.c

#	These targets are useful but optional

partslist:
	@echo chrtbl.mk chrtbl.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo chrtbl | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit chrtbl.mk $(LOCALINCS) chrtbl.c -o chrtbl.o chrtbl
