#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)fuser:fuser.mk	1.1.13.4"
#ident "$Header: fuser.mk 1.3 91/05/22 $"

include $(CMDRULES)

#	Makefile for fuser

OWN = bin
GRP = bin

all: fuser

fuser: fuser.o
	$(CC) -o fuser fuser.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

fuser.o: fuser.c \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/pwd.h \
	$(INC)/stdio.h \
	$(INC)/priv.h $(INC)/sys/privilege.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/param.h \
	$(INC)/sys/var.h \
	$(INC)/sys/utssys.h \
	$(INC)/sys/ksym.h

install: all
	-rm -f $(ETC)/fuser
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) fuser
	-$(SYMLINK) /usr/sbin/fuser $(ETC)/fuser

clean:
	rm -f fuser.o

clobber: clean
	rm -f fuser

lintit:
	$(LINT) $(LINTFLAGS) fuser.c

#	These targets are useful but optional

partslist:
	@echo fuser.mk fuser.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo fuser | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit fuser.mk $(LOCALINCS) fuser.c -o fuser.o fuser
