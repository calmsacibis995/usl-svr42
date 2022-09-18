#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)acct:common/cmd/acct/lib/Makefile	1.8.3.4"
#ident "$Header: $"

include $(CMDRULES)

LIB = a.a
LOCALINC = -I ..
LOCALDEF = -D_LTYPES
SOURCES = copyn.c expand.c namtouid.c substr.c tmsecs.c devtolin.c lintodev.c \
	pnpsplit.c tmless.c uidtonam.c
OBJECTS = $(SOURCES:.c=.o)

all: $(LIB)

$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $(OBJECTS)
	@echo lib $(LIB) is now up to date

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f $(LIB)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
