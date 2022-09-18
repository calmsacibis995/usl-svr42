#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)who:who.mk	1.9.5.3"
#ident "$Header: who.mk 1.3 91/04/29 $"

include $(CMDRULES)

#	Makefile for who 

OWN = bin
GRP = bin

all: who

who: who.o 
	$(CC) -o who who.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

who.o:  $(INC)/errno.h $(INC)/sys/errno.h \
	 $(INC)/fcntl.h $(INC)/stdio.h \
	 $(INC)/string.h $(INC)/sys/types.h \
	 $(INC)/unistd.h $(INC)/stdlib.h \
	 $(INC)/sys/stat.h $(INC)/time.h \
	 $(INC)/utmp.h $(INC)/locale.h \
	 $(INC)/pfmt.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) who
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) who

clean:
	rm -f who.o 

clobber: clean
	rm -f who

lintit:
	$(LINT) $(LINTFLAGS) who.c

# These targets are useful but optional

partslist:
	@echo who.mk who.c c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo who | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit who.mk $(LOCALINCS) who.c -o who.o who
