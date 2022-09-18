#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libia:common/lib/libia/libia.mk	1.1.5.2"
#ident "$Header: libia.mk 1.4 91/06/21 $"

include $(LIBRULES)

OWN=root
GRP=sys

OBJECTS = alias.o events.o getadtent.o getia.o lvlia.o
LIBRARY=libia.a

all:	$(LIBRARY)

$(LIBRARY):	$(OBJECTS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJECTS)

alias.o: alias.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mac.h \
	$(INC)/ia.h \
	$(INC)/audit.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h \
	$(INC)/stdlib.h \
	$(INC)/sys/uio.h \
	$(INC)/unistd.h

events.o: events.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/param.h \
	$(INC)/audit.h \
	$(INC)/pfmt.h \
	$(INC)/locale.h \
	$(INC)/stdlib.h

getadtent.o: getadtent.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/ia.h \
	$(INC)/string.h \
	$(INC)/errno.h \
	$(INC)/audit.h

getia.o: getia.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/audit.h \
	$(INC)/ia.h \
	$(INC)/search.h \
	$(INC)/fcntl.h \
	$(INC)/memory.h \
	$(INC)/sys/mman.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mac.h

lvlia.o: lvlia.c \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/mac.h \
	$(INC)/ia.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/fcntl.h \
	$(INC)/sys/stat.h

install: all
	$(INS) -f $(USRLIB) -m 0500 -u $(OWN) -g $(GRP) $(LIBRARY)

clean:
	-rm -f *.o

clobber:	clean
	-rm -f $(LIBRARY)

