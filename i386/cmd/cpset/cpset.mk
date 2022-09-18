#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cpset:cpset.mk	1.1.5.2"
#ident  "$Header: cpset.mk 1.1 91/05/17 $"

include $(CMDRULES)

# Makefile for cpset
# to install when not privileged
# set $(CH) in the environment to #

OWN = bin
GRP = bin

all: cpset

cpset: cpset.o
	$(CC) -o cpset cpset.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

cpset.o: cpset.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/errno.h \
	$(INC)/string.h

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) cpset

clean:
	rm -f cpset.o

clobber: clean
	rm -f cpset

lintit: 
	$(LINT) $(LINTFLAGS) cpset.c
