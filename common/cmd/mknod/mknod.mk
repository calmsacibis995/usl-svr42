#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)mknod:mknod.mk	1.6.7.3"
#ident "$Header: mknod.mk 1.2 91/04/17 $"

include $(CMDRULES)

#	Makefile for mknod 

OWN = bin
GRP = bin

all: mknod

mknod: mknod.o 
	$(CC) -o mknod mknod.o  $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(CC) -o mknod.dy mknod.o  $(LDFLAGS) $(LDLIBS)

mknod.o: mknod.c \
	$(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/mkdev.h

install: all
	-rm -f $(ETC)/mknod
	-rm -f $(ETC)/mknod.dy
	-rm -f $(USRSBIN)/mknod
	-rm -f $(USRSBIN)/mknod.dy
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) mknod
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) mknod.dy
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) mknod
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) mknod.dy
	$(SYMLINK) /sbin/mknod $(ETC)/mknod

clean:
	rm -f mknod.o

clobber: clean
	rm -f mknod mknod.dy

lintit:
	$(LINT) $(LINTFLAGS) mknod.c

#	These targets are useful but optional

partslist:
	@echo mknod.mk mknod.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(SBIN) | tr ' ' '\012' | sort

product:
	@echo mknod | tr ' ' '\012' | \
	sed 's;^;$(SBIN)/;'

srcaudit:
	@fileaudit mknod.mk $(LOCALINCS) mknod.c -o mknod.o mknod mknod.dy
