#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pt_chmod:pt_chmod.mk	1.1.7.2"
#ident  "$Header: pt_chmod.mk 1.3 91/06/28 $"

include $(CMDRULES)


OWN = root
GRP = bin

LDLIBS = -ladm
OBJECTS = pt_chmod.o

#
# Header dependencies
#

all: pt_chmod

pt_chmod: pt_chmod.o
	$(CC) pt_chmod.o -o pt_chmod $(LDFLAGS) $(LDLIBS) $(SHLIBS)

pt_chmod.o: pt_chmod.c \
	$(INC)/string.h \
	$(INC)/stdio.h \
	$(INC)/grp.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/types.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h

install: all
	$(INS) -f $(USRLIB) -m 04111 -u $(OWN) -g $(GRP) pt_chmod

clean:
	-rm -f $(OBJECTS)

clobber: clean
	-rm -f pt_chmod

lintit:
	$(LINT) $(LINTFLAGS) pt_chmod.c
