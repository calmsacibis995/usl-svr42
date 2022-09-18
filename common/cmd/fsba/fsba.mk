#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)fsba:common/cmd/fsba/fsba.mk	1.5.5.2"
#ident "$Header: fsba.mk 1.4 91/07/08 $"

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin

OBJS = fsba.o bsize.o

all: fsba

fsba:	$(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(SHLIBS)

fsba.o:	fsba.c \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/fcntl.h \
	fsba.h

bsize.o: bsize.c \
	$(INC)/sys/types.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5dir.h \
	fsba.h

install: all
	-rm -f $(ETC)/fsba
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) fsba
	-$(SYMLINK) /usr/sbin/fsba $(ETC)/fsba

clean:
	rm -f *.o

clobber: clean
	rm -f fsba
