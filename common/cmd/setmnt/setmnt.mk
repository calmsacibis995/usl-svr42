#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)setmnt:setmnt.mk	1.9.8.2"
#ident "$Header: setmnt.mk 1.2 91/03/20 $"

include $(CMDRULES)

#	Makefile for setmnt 

OWN = bin
GRP = bin

all: setmnt

setmnt: setmnt.o 
	$(CC) -o setmnt setmnt.o  $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

setmnt.o: setmnt.c \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statvfs.h \
	$(INC)/mac.h

install: all
	-rm -f $(ETC)/setmnt
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) setmnt
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) setmnt
	-$(SYMLINK) /sbin/setmnt $(ETC)/setmnt

clean:
	rm -f setmnt.o

clobber: clean
	rm -f setmnt

lintit:
	$(LINT) $(LINTFLAGS) setmnt.c

#	These targets are useful but optional

partslist:
	@echo setmnt.mk setmnt.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo setmnt | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit setmnt.mk $(LOCALINCS) setmnt.c -o setmnt.o setmnt
