#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)killall:killall.mk	1.5.7.2"
#ident "$Header: killall.mk 1.2 91/04/19 $"

include $(CMDRULES)

#	Makefile for killall 

OWN = bin
GRP = bin

all: killall

killall: killall.o 
	$(CC) -o killall killall.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

killall.o: killall.c \
	$(INC)/sys/types.h \
	$(INC)/sys/procset.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h $(INC)/sys/privilege.h

install: all
	-rm -f $(ETC)/killall
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) killall
	-$(SYMLINK) /usr/sbin/killall $(ETC)/killall

clean:
	rm -f killall.o

clobber: clean
	rm -f killall

lintit:
	$(LINT) $(LINTFLAGS) killall.c

#	These targets are useful but optional

partslist:
	@echo killall.mk killall.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo killall | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit killall.mk $(LOCALINCS) killall.c -o killall.o killall
