#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)mv:mv.mk	1.16.6.24"
#ident "$Header: mv.mk 1.4 91/06/20 $"

include $(CMDRULES)

#	Makefile for mv/cp/ln

OWN = bin
GRP = bin

LIST = lp

MAINS = mv ln cp mv.dy ln.dy cp.dy

all: 	mv
	-rm -f cp ln cp.dy ln.dy
	@/bin/ln mv cp
	@/bin/ln mv ln
	@/bin/ln mv.dy cp.dy
	@/bin/ln mv.dy ln.dy

mv: mv.o
	$(CC) -o mv mv.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(CC) -o mv.dy mv.o $(LDFLAGS) $(LDLIBS)

mv.o: mv.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/utime.h \
	$(INC)/signal.h \
	$(INC)/errno.h \
	$(INC)/sys/param.h \
	$(INC)/dirent.h \
	$(INC)/stdlib.h \
	$(INC)/acl.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/priv.h

install: all
	-rm -f $(USRBIN)/ln $(USRBIN)/cp
	-rm -f $(USRBIN)/ln.dy $(USRBIN)/cp.dy
	-rm -f $(SBIN)/ln $(SBIN)/cp
	-rm -f $(SBIN)/ln.dy $(SBIN)/cp.dy
	 $(INS) -o -m 0555 -u $(OWN) -g $(GRP) -f $(USRBIN) mv
	 $(INS) -o -m 0555 -u $(OWN) -g $(GRP) -f $(SBIN) mv
	 $(INS) -o -m 0555 -u $(OWN) -g $(GRP) -f $(USRBIN) mv.dy
	 $(INS) -o -m 0555 -u $(OWN) -g $(GRP) -f $(SBIN) mv.dy
	/bin/ln $(USRBIN)/mv $(USRBIN)/ln
	/bin/ln $(USRBIN)/mv $(USRBIN)/cp
	/bin/ln $(USRBIN)/mv.dy $(USRBIN)/ln.dy
	/bin/ln $(USRBIN)/mv.dy $(USRBIN)/cp.dy
	/bin/ln $(SBIN)/mv $(SBIN)/ln
	/bin/ln $(SBIN)/mv $(SBIN)/cp
	/bin/ln $(SBIN)/mv.dy $(SBIN)/ln.dy
	/bin/ln $(SBIN)/mv.dy $(SBIN)/cp.dy

clean:
	rm -f mv.o

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) mv.c

#	These targets are useful but optional

partslist:
	@echo mv.mk mv.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo $(MAINS) | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit mv.mk $(LOCALINCS) mv.c -o mv.o $(MAINS)

listing:
	pr mv.mk $(SOURCE) | $(LIST)

listmk: 
	pr mv.mk | $(LIST)
