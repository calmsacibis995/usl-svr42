#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)fd.cmds:fdfs/fdfs.mk	1.2.3.2"
#ident "$Header: fdfs.mk 1.2 91/08/07 $"

include $(CMDRULES)

INSDIR = $(ETC)/fs/fdfs
FRC =

all:	mount 

mount:	mount.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

mount.o:mount.c\
	$(INC)/stdio.h\
	$(INC)/signal.h\
	$(INC)/unistd.h\
	$(INC)/errno.h\
	$(INC)/sys/mnttab.h\
	$(INC)/sys/mount.h\
	$(INC)/sys/types.h\
	$(FRC)

install: all
	[ -d $(ETC)/fs ] || mkdir $(ETC)/fs
	[ -d $(INSDIR) ] || mkdir $(INSDIR)
	[ -d $(USRLIB)/fs/fdfs ] || mkdir -p $(USRLIB)/fs/fdfs
	$(INS) -f $(INSDIR) mount
	cp $(INSDIR)/mount $(USRLIB)/fs/fdfs/mount
	cp /dev/null fsck
	$(INS) -f $(INSDIR) fsck
	cp $(INSDIR)/fsck $(USRLIB)/fs/fdfs/fsck

clean:
	rm -f *.o

clobber:	clean
	rm -f mount
FRC:
