#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)mkpart:i386/cmd/mkpart/mkpart.mk	1.1.2.3"
#ident	"$Header: mkpart.mk 1.2 91/07/10 $"

include $(CMDRULES)


OWN = root
GRP = bin

OBJECTS = mkpart.o mkboot.o mkdevice.o scan.o y.tab.o
YFILES  = y.output y.tab.c y.tab.h
YFLAGS  = -vd
LDLIBS  = $(LIBELF)

all:  mkpart

mkpart: $(OBJECTS)
	$(CC) -o mkpart $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

mkboot.o: mkboot.c \
	$(INC)/stdio.h \
	$(INC)/sys/fcntl.h \
	$(INC)/a.out.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/fdisk.h \
	$(INC)/libelf.h \
	mkpart.h

mkdevice.o: mkdevice.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/alttbl.h \
	$(INC)/sys/fdisk.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/sdi_edt.h \
	mkpart.h \
	parse.h

mkpart.o: mkpart.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	mkpart.h \
	parse.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/fdisk.h

scan.o: scan.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	mkpart.h \
	parse.h \
	y.tab.h \
	$(INC)/ctype.h

y.tab.o: partitions.y \
	mkpart.h \
	parse.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vtoc.h \
	$(INC)/stdio.h \
	$(INC)/malloc.h \
	$(INC)/memory.h \
	$(INC)/values.h
	$(YACC) $(YFLAGS) partitions.y
	$(CC) $(CFLAGS) $(DEFLIST) -c y.tab.c

y.tab.h:    y.tab.o

clean:
	rm -f $(OBJECTS) $(YFILES)

clobber: clean
	rm -f mkpart

lintit:
	$(LINT) $(LINTFLAGS) *.c

install: all
	$(INS) -f $(SBIN) -m 0500 -u $(OWN) -g $(GRP) mkpart

