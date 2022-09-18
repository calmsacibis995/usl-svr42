#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ps:ps.mk	1.7.23.4"
#ident "$Header: ps.mk 1.2 91/04/16 $"

include $(CMDRULES)


OWN = bin
GRP = bin

LDLIBS = -lcmd -lw

all: ps

ps: ps.o
	$(CC) -o ps ps.o $(LDFLAGS) $(LDLIBS) 

ps.o: ps.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/pwd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/ftw.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/limits.h \
	$(INC)/sys/mnttab.h \
	$(INC)/dirent.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/fault.h \
	$(INC)/sys/syscall.h \
	$(INC)/sys/time.h \
	$(INC)/sys/procfs.h \
	$(INC)/locale.h \
	$(INC)/sys/proc.h \
	$(INC)/mac.h \
	$(INC)/deflt.h \
	$(INC)/priv.h \
	$(INC)/pfmt.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) ps

clean:
	-rm -f ps.o

clobber: clean
	rm -f ps

lintit:
	$(LINT) $(LINTFLAGS) *.c
