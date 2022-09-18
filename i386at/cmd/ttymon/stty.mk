#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ttymon:i386at/cmd/ttymon/stty.mk	1.13.7.2"
#ident "$Header: stty.mk 1.1 91/05/27 $"

include $(CMDRULES)

#	Makefile for stty 

OWN = root
GRP = sys

OBJECTS = stty.o sttytable.o sttyparse.o
SOURCES = $(OBJECTS:.o=.c)

LOCALDEF = -DMERGE386

all: stty

stty: $(OBJECTS)
	$(CC) -o stty $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

stty.o: stty.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/locale.h \
	$(INC)/sys/types.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/stermio.h \
	$(INC)/sys/termiox.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	stty.h

sttytable.o: sttytable.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/sys/stermio.h \
	$(INC)/sys/termiox.h \
	stty.h

sttyparse.o: sttyparse.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/sys/types.h \
	$(INC)/ctype.h \
	$(INC)/termio.h $(INC)/sys/termio.h \
	$(INC)/priv.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/stermio.h \
	$(INC)/sys/termiox.h \
	stty.h \
	$(INC)/pfmt.h \
	$(INC)/sys/ioctl.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f stty

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) stty

#	These targets are useful but optional

partslist:
	@echo stty.mk $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo stty | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit stty.mk $(LOCALINCS) $(SOURCES) -o $(OBJECTS) stty
