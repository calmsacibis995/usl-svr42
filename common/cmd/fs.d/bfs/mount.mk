#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)bfs.cmds:bfs/mount.mk	1.8.9.4"
#ident "$Header: mount.mk 1.2 91/04/11 $"

include $(CMDRULES)

OWN = bin
GRP = bin

FRC =

FILES =\
	mount.o

all: mount

mount: $(FILES)
	$(CC) $(LDFLAGS) -o $@ $(FILES) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o $@.dy $(FILES) $(LDLIBS) $(SHLIBS)

install: mount
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(ETC)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mount.dy
	$(INS) -f $(USRLIB)/fs/bfs -m 0555 -u $(OWN) -g $(GRP) mount.dy

clean:
	rm -f *.o

clobber: clean
	rm -f mount mount.dy

#
# Header dependencies
#

mount.o: mount.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/mount.h \
	$(INC)/sys/types.h \
	$(FRC)
