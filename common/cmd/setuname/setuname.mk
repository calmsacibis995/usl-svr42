#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)setuname:setuname.mk	1.2.5.3"
#ident "$Header: setuname.mk 1.3 91/05/23 $"

include $(CMDRULES)

#makefile for setuname

OWN = bin
GRP = bin

LDLIBS = $(LIBELF)

all:  setuname

setuname: setuname.o
	$(CC) -o setuname setuname.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

setuname.o: setuname.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/string.h \
	$(INC)/fmtmsg.h \
	$(INC)/ctype.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/ksym.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) setuname 

clean:
	rm -f setuname.o

clobber: clean
	rm -f setuname

lintit:
	$(LINT) $(LINTFLAGS) setuname.c

#	These targets are useful but optional

partslist:
	@echo setuname.mk setuname.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo setuname | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit setuname.mk $(LOCALINCS) setuname.c -o setuname.o setuname
