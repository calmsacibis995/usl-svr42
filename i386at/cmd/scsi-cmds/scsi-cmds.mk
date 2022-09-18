#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)scsi-cmds:i386at/cmd/scsi-cmds/scsi-cmds.mk	1.3"
#ident  "$Header: $"

include $(CMDRULES)

INSDIR = $(ETC)/scsi
FMTDIR = $(ETC)/scsi/format.d
MDVDIR = $(ETC)/scsi/mkdev.d
SYSTEMENV = 4 
FORMAT_FILES = sd00.0 sd01.1
MKDEV_FILES = disk1 qtape1 cdrom1 worm1 9track1

SCSICMDS = scsiformat DISK sc01qa sw01qa tapecntl

FORMAT = \
        tc.index \
        format.d/sd00.0 \
        format.d/sd01.1

MKDEV = \
	mkdev.d/disk1 \
	mkdev.d/qtape1 \
	mkdev.d/cdrom1 \
	mkdev.d/worm1

all:	$(SCSICMDS) $(FORMAT) $(MKDEV)
	echo "*** scsi command build completed *** " > /dev/null

install:	all
	[ -d $(INSDIR) ] || mkdir $(INSDIR)
	[ -d $(FMTDIR) ] || mkdir $(FMTDIR)
	[ -d $(MDVDIR) ] || mkdir $(MDVDIR)
	[ -d $(USRBIN) ] || mkdir -p $(USRBIN)
	[ -d tmp ] || mkdir tmp
	$(INS) -f $(INSDIR) -m 0755 -u bin -g bin scsiformat
	$(INS) -f $(INSDIR) -m 0755 -u bin -g bin sc01qa
	$(INS) -f $(INSDIR) -m 0755 -u bin -g bin sw01qa
	$(INS) -f $(USRBIN) -m 0555 -u bin -g bin tapecntl
	$(INS) -f $(FMTDIR) -m 0555 -u bin -g bin DISK
	grep -v "^#ident" tc.index > tmp/tc.index ;\
	$(INS) -f $(INSDIR) -m 0555 -u bin -g bin tmp/tc.index
	for i in $(FORMAT_FILES); \
	do \
		grep -v "^#ident" format.d/$$i > tmp/$$i ;\
		install -f $(FMTDIR) -m 0555 -u bin -g bin tmp/$$i ;\
	done
	for i in $(MKDEV_FILES); \
	do \
		grep -v "^#ident" mkdev.d/$$i > tmp/$$i ;\
		install -f $(MDVDIR) -m 0555 -u bin -g bin tmp/$$i ;\
	done

clean:
	rm -f *.o tmp/* $(SCSICMDS)

clobber: clean
	rm -f $(SCSICMDS)
	rmdir tmp

scsiformat: format.o scl.o script.o scsicomm.o
	$(CC) -o scsiformat format.o scl.o script.o scsicomm.o

DISK: DISK.o scl.o script.o scsicomm.o
	$(CC) -o DISK DISK.o scl.o script.o scsicomm.o

sc01qa:		sc01qa.o
	$(CC) -o sc01qa sc01qa.o

sw01qa:		sw01qa.o
	$(CC) -o sw01qa sw01qa.o

tapecntl:	tapecntl.o
	$(CC) -o tapecntl tapecntl.o

format.o: format.c \
	$(INC)/stdio.h \
	$(INC)/utmp.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/tokens.h \
	$(INC)/sys/scl.h 

scl.o:	scl.c \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/errno.h \
	$(INC)/fcntl.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/scsicomm.h \
	$(INC)/string.h \
	$(INC)/signal.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/scsi.h \
	$(INC)/sys/sdi.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/tokens.h \
	$(INC)/sys/badsec.h \
	$(INC)/sys/scl.h

script.o: script.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/scsi.h \
	$(INC)/sys/tokens.h

DISK.o: DISK.c \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/mnttab.h \
	$(INC)/utmp.h \
	$(INC)/sys/types.h \
	$(INC)/signal.h \
	$(INC)/sys/signal.h \
	$(INC)/fcntl.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/filsys.h \
	$(INC)/sys/fs/s5macros.h \
	$(INC)/sys/fdisk.h \
	$(INC)/sys/scsi.h \
	$(INC)/sys/sd01_ioctl.h \
	$(INC)/sys/mirror.h \
	$(INC)/sys/scl.h \
	$(INC)/sys/tokens.h \
	$(INC)/sys/badsec.h \
	$(INC)/sys/scsicomm.h 

scsicomm.o: scsicomm.c \
	$(INC)/sys/types.h \
	$(INC)/sys/mkdev.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sdi_edt.h \
	$(INC)/sys/mirror.h \
	$(INC)/fcntl.h \
	$(INC)/sys/fcntl.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/vtoc.h \
	$(INC)/sys/sd01_ioctl.h \
	$(INC)/string.h \
	$(INC)/stdio.h \
	$(INC)/sys/scsicomm.h 

sc01qa.o:	sc01qa.c

sw01qa.o:	sw01qa.c

tapecntl.o:	tapecntl.c

