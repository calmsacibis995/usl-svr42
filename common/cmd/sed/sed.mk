#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sed:sed.mk	1.15.1.5"
#ident "$Header: sed.mk 1.3 91/04/29 $"

include $(CMDRULES)

#	Makefile for sed

OWN = bin
GRP = bin

LDLIBS = -lgen -lw

SOURCES = sed0.c sed1.c
OBJECTS = $(SOURCES:.c=.o)

all: sed

sed: $(OBJECTS)
	$(CC) -o sed $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

sed0.o: sed0.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	sed.h \
	$(INC)/stdlib.h \
	$(INC)/limits.h \
	$(INC)/regexpr.h

sed1.o: sed1.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	sed.h \
	$(INC)/stdlib.h \
	$(INC)/limits.h \
	$(INC)/regexpr.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) sed

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f sed

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

#	These targets are useful but optional

partslist:
	@echo sed.mk $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo sed | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit sed.mk $(LOCALINCS) $(SOURCES) -o $(OBJECTS) sed
