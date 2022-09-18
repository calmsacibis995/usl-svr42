#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)swap:swap.mk	1.12.2.3"
#ident "$Header: swap.mk 1.4 91/04/11 $"

include $(CMDRULES)

OWN = bin
GRP = sys

all: swap

swap: swap.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/param.h \
	$(INC)/dirent.h \
	$(INC)/sys/swap.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/uadmin.h \
	$(INC)/vm/anon.h \
	$(INC)/fcntl.h \
	$(INC)/sys/ksym.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	-rm -f $(ETC)/swap
	 $(INS) -f $(USRSBIN) -m 02755 -u $(OWN) -g $(GRP) swap
	-$(SYMLINK) /usr/sbin/swap $(ETC)/swap

clean:
	-rm -f swap.o

clobber: clean
	-rm -f swap
	
lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
