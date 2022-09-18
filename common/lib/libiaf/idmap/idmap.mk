#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libiaf:common/lib/libiaf/idmap/idmap.mk	1.2.2.3"
#ident "$Header: idmap.mk 1.3 91/03/14 $"

include $(LIBRULES)
LOCALDEF = $(PICFLAG)

SOURCES = namemap.c attrmap.c
OBJECTS = namemap.o attrmap.o
LIBOBJECTS = ../namemap.o ../attrmap.o
SRCS = $(OBJS:%.o=%.c)

all:	$(OBJECTS)
	cp $(OBJECTS) ../


namemap.o:	namemap.c \
		$(INC)/pwd.h \
		$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/sys/types.h \
		idmap.h \
		gmatch.c \
		$(INC)/stdlib.h \
		$(INC)/limits.h \
		$(INC)/ctype.h \
		_range.h \
		_wchar.h \
		breakname.c

attrmap.o:	attrmap.c \
		$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/sys/types.h \
		idmap.h \
		gmatch.c \
		$(INC)/stdlib.h \
		$(INC)/limits.h \
		$(INC)/ctype.h \
		_range.h \
		_wchar.h \
		breakname.c


lintit:
	$(LINT) -I$(INC) $(SOURCES)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(LIBOBJECTS)

size: all
	$(SIZE) $(LIBOBJECTS)

strip: all
	$(STRIP) $(LIBOBJECTS)
