#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pkgtools:include.mk	1.3"

include $(LIBRULES)

.c.a:;

LIBSTUBS = libstubs.a
OBJECTS = $(SOURCES:.c=.o)

SOURCES = stubs.c


all: $(LIBSTUBS)

.PRECIOUS: $(LIBSTUBS)

$(LIBSTUBS): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBSTUBS) $(OBJECTS)
