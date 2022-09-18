#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bfs.cmds:bfs/mkfs.mk	1.7.7.3"
#ident "$Header: mkfs.mk 1.2 91/04/11 $"

include $(CMDRULES)

INCSYS = $(INC)
OWN = bin
GRP = bin
FRC =

FILES =\
	mkfs.o

all: mkfs

mkfs: $(FILES)
	$(CC) $(LDFLAGS) -o mkfs $(FILES) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o mkfs.dy $(FILES) $(LDLIBS) $(SHLIBS)

install: mkfs
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mkfs
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mkfs
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mkfs.dy
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mkfs.dy

clean:
	rm -f *.o

clobber: clean
	rm -f mkfs mkfs.dy *.o
#
# Header dependencies
#

mkfs.o: mkfs.c \
	$(INC)/stdio.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/vnode.h \
	$(INCSYS)/sys/fs/bfs.h \
	$(INCSYS)/sys/vtoc.h \
	$(INCSYS)/sys/stat.h \
	$(INCSYS)/sys/fcntl.h \
	$(FRC)
