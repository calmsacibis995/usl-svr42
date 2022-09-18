#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)mkmsgs:mkmsgs.mk	1.6.8.2"
#ident  "$Header: mkmsgs.mk 1.3 91/06/28 $"

include $(CMDRULES)

#	Makefile for mkmsgs

OWN = root
GRP = root

LDLIBS = -lgen

all: mkmsgs

mkmsgs: mkmsgs.o 
	$(CC) -o mkmsgs mkmsgs.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

mkmsgs.o: mkmsgs.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/priv.h \
	$(INC)/mac.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/unistd.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) mkmsgs

clean:
	rm -f mkmsgs.o

clobber: clean
	rm -f mkmsgs

lintit:
	$(LINT) $(LINTFLAGS) mkmsgs.c

#	These targets are useful but optional

partslist:
	@echo mkmsgs.mk mkmsgs.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo mkmsgs | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit mkmsgs.mk $(LOCALINCS) mkmsgs.c -o mkmsgs.o mkmsgs
