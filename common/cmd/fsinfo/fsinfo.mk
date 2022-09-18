#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)fsinfo:common/cmd/fsinfo/fsinfo.mk	1.2.6.2"
#ident "$Header: fsinfo.mk 1.3 91/07/08 $"
#
#		Copyright 1984 AT&T
#

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/lbin
OWN = bin
GRP = bin

OBJS = fsinfo.o bsize.o

all:	fsinfo

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) fsinfo

fsinfo: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(NOSHLIBS)

clean:
	-rm -f fsinfo.o

clobber: clean
	-rm -f fsinfo

FRC:

#
# Header dependencies
#

fsinfo.o: fsinfo.c \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5dir.h \
	$(INC)/fcntl.h \
	$(FRC)

bsize.o: bsize.c \
	$(INC)/sys/types.h \
	$(INC)/sys/fs/s5ino.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/fs/s5dir.h
