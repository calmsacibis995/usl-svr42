#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)localedef:common/cmd/localedef/colltbl/colltbl.mk	1.1.9.2"
#ident "$Header: colltbl.mk 1.5 91/06/21 $"

include $(CMDRULES)

#	Makefile for colltbl

OWN = bin
GRP = bin

LDLIBS = -ly
YFLAGS = -d

OBJECTS = collfcns.o colltbl.o diag.o parse.o lex.o
SOURCES = $(OBJECTS:.o=.c)

.MUTEX: y.tab.h parse.c

all: colltbl

colltbl: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

collfcns.o: collfcns.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/stdlib.h \
	$(INC)/stddef.h \
	colltbl.h \
	$(INC)/regexp.h

colltbl.o: colltbl.c \
	$(INC)/stdio.h \
	colltbl.h

diag.o: diag.c \
	$(INC)/stdio.h \
	$(INC)/varargs.h \
	colltbl.h

lex.o: lex.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	colltbl.h \
	y.tab.h

parse.o: parse.c \
	colltbl.h \
	$(INC)/malloc.h \
	$(INC)/memory.h \
	$(INC)/values.h

parse.c y.tab.h: parse.y
	$(YACC) $(YFLAGS) parse.y
	mv y.tab.c parse.c

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) colltbl ;\
	$(INS) -f $(USRLIB)/locale/C -m 0555 -u $(OWN) -g $(GRP) colltbl_C ;\
	$(CH)./colltbl colltbl_C ;\
	$(CH)$(INS) -f $(USRLIB)/locale/C LC_COLLATE ;\
	$(CH)rm -f LC_COLLATE

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f colltbl y.tab.h parse.c

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

#	These targets are useful but optional

partslist:
	@echo colltbl.mk $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo  | tr ' ' '\012' | sort

product:
	@echo colltbl | tr ' ' '\012' | \
	sed 's;^;/;'

srcaudit:
	@fileaudit colltbl.mk $(LOCALINCS) $(SOURCES) -o $(OBJECTS) colltbl
