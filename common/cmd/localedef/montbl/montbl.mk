#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)localedef:common/cmd/localedef/montbl/montbl.mk	1.1.7.2"
#ident "$Header: montbl.mk 1.2 91/04/17 $"

include $(CMDRULES)

#	Makefile for montbl

OWN = bin
GRP = bin

all: montbl

montbl: montbl.o 
	$(CC) -o montbl montbl.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

montbl.o: montbl.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/locale.h \
	$(INC)/limits.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) montbl ;\
	$(INS) -f $(USRLIB)/locale/C -m 0555 -u $(OWN) -g $(GRP) montbl_C ;\
	$(CH)./montbl montbl_C ;\
	$(CH)$(INS) -f $(USRLIB)/locale/C LC_MONETARY ;\
	$(CH)rm -f LC_MONETARY

clean:
	rm -f montbl.o

clobber: clean
	rm -f montbl

lintit:
	$(LINT) $(LINTFLAGS) montbl.c

#	These targets are useful but optional

partslist:
	@echo montbl.mk montbl.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo montbl | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit montbl.mk $(LOCALINCS) montbl.c -o montbl.o montbl
