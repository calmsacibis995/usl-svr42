#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)format:i386/cmd/format/format.mk	1.4.6.2"
#ident	"$Header: format.mk 1.3 91/08/08 $"

include $(CMDRULES)

INSDIR = $(USRSBIN)
LOCALDEF= -D$(MACH)

FRC =

all:	format

install: all
	-rm -f $(ETC)/format
	$(INS) -f $(INSDIR) format
	$(SYMLINK) /usr/sbin/format $(ETC)/format

format: format.o devcheck.o
	$(CC) -o $@ format.o devcheck.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

devcheck.o: devcheck.c \
	$(INC)/stdio.h

format.o: format.c \
	$(INC)/sys/types.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/param.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/iobuf.h \
	$(INC)/sys/vtoc.h \
	$(INC)/stdio.h \
	$(INC)/errno.h

clean:
	-rm -f *.o

clobber: clean
	-rm -f format

FRC:

#
# Header dependencies
#

format: format.c \
	$(INC)/sys/types.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/iobuf.h \
	$(INC)/errno.h \
	$(INC)/sys/vtoc.h \
	$(FRC)
