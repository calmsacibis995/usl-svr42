#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ls:ls.mk	1.13.5.18"
#ident "$Header: ls.mk 1.2 91/04/09 $"

include $(CMDRULES)

#	Makefile for ls

OWN = bin
GRP = bin

LDLIBS = -lgen -lcmd -lw

all: ls

ls: ls.o
	$(CC) -o ls ls.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

ls.o: ls.c \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/dirent.h \
	$(INC)/string.h \
	$(INC)/locale.h \
	$(INC)/sys/euc.h \
	$(INC)/getwidth.h \
	$(INC)/curses.h \
	$(INC)/termios.h \
	$(INC)/acl.h \
	$(INC)/mac.h \
	$(INC)/deflt.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h \
	$(INC)/term.h \
	$(INC)/sys/statvfs.h \
	$(INC)/sys/fs/vx_ioctl.h \
	$(INC)/fcntl.h
clean:
	rm -f ls.o

clobber: clean
	rm -f ls

lintit:
	$(LINT) $(LINTFLAGS) ls.c

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ls

#	These targets are useful but optional

partslist:
	@echo ls.mk ls.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo ls | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit ls.mk $(LOCALINCS) ls.c -o ls.o ls
