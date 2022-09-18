#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)pdi.cmds:pdi.cmds.mk	1.2.2.8"
#ident  "$Header: miked 4/6/92$"

include $(CMDRULES)

#       Makefile for pdi.cmds

OWN = root
GRP = sys

INSDIR = $(ETC)/scsi
LDLIBS = -ladm -lelf -lgen
MAINS = pdimkdev pdiconfig diskcfg pdiadd
MOBJECTS = mkdev.o scsicomm.o
COBJECTS = config.o scsicomm.o
DOBJECTS = diskcfg.o scsicomm.o
SOBJECTS = pdiadd.sh

all:	$(MAINS)

install:	all
	-[ -d $(INSDIR) ] || mkdir $(INSDIR)
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) diskcfg
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) pdiconfig
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) pdimkdev
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) pdiadd
	-$(RM) -f $(INSDIR)/pdimkdtab
	-ln $(INSDIR)/pdimkdev $(INSDIR)/pdimkdtab
	-$(RM) -f $(INSDIR)/pdirm
	-ln $(INSDIR)/pdiadd $(INSDIR)/pdirm
clean:
	rm -f *.o pdirm pdimkdtab $(MAINS)

clobber: clean

pdiadd: $(SOBJECTS)

pdimkdev: $(MOBJECTS)
	$(CC) -o pdimkdev $(MOBJECTS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

diskcfg: $(DOBJECTS)
	$(CC) -o diskcfg $(DOBJECTS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

pdiconfig: $(COBJECTS)
	$(CC) -o pdiconfig $(COBJECTS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

config.o: config.c \
	diskcfg.h \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/limits.h \
	$(INC)/dirent.h \
	$(INC)/nlist.h \
	$(INC)/fcntl.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/sdi.h \
	$(INC)/sys/scsicomm.h

mkdev.o: mkdev.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/statfs.h \
	$(INC)/ctype.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/buf.h \
	$(INC)/sys/vtoc.h \
	$(INC)/string.h \
	$(INC)/ftw.h \
	$(INC)/devmgmt.h \
	$(INC)/unistd.h \
	$(INC)/sys/vfstab.h \
	$(INC)/sys/sd01_ioctl.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/fs/s5filsys.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/scsi.h \
	$(INC)/sys/sdi.h \
	$(INC)/sys/scsicomm.h

scsicomm.o: scsicomm.c \
	$(INC)/sys/types.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/fcntl.h \
	$(INC)/sys/fcntl.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/sd01_ioctl.h \
	$(INC)/string.h \
	$(INC)/stdio.h \
	$(INC)/sys/scsicomm.h 

diskcfg.o: diskcfg.c \
	diskcfg.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/unistd.h \
	$(INC)/limits.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/scsicomm.h \
	$(INC)/sys/types.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/stat.h

