#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rm:rm.mk	1.5.7.3"
#ident "$Header: rm.mk 1.2 91/03/22 $"

include $(CMDRULES)

#	Makefile for rm

OWN = bin
GRP = bin

all: rm

rm: rm.o
	$(CC) -o rm rm.o $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)
	$(CC) -o rm.dy rm.o $(LDFLAGS) $(LDLIBS) 

rm.o: rm.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/dirent.h \
	$(INC)/limits.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) rm
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) rm.dy

clean:
	rm -f rm.o

clobber: clean
	rm -f rm rm.dy

lintit:
	$(LINT) $(LINTFLAGS) rm.c

#	These targets are useful but optional

partslist:
	@echo rm.mk rm.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo rm | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit rm.mk $(LOCALINCS) rm.c -o rm.o rm rm.dy
