#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dfsck:dfsck.mk	1.4.3.2"

include	$(CMDRULES)

MAINS	= dfsck
OBJECTS = dfsck.o

ALL:		$(MAINS)

$(MAINS):	dfsck.o
		$(CC) -o dfsck dfsck.o $(LDFLAGS)

dfsck.o:	 $(INC)/stdio.h \
		 $(INC)/fcntl.h \
		 $(INC)/signal.h \
		 $(INC)/sys/signal.h \
		 $(INC)/errno.h \
		 $(INC)/sys/errno.h \
		 $(INC)/sys/types.h \
		 $(INC)/sys/stat.h 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(USRSBIN) -m 0555 -u bin -g bin $(MAINS)
