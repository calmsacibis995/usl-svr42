#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)disksetup:i386at/cmd/disksetup/disksetup.mk	1.3.7.10"
#ident	"$Header: $"

include	$(CMDRULES)

BUS	= AT386
LOCALDEF= -D$(BUS)

OFILES = script.o scsicomm.o scl.o scsi_setup.o ix_altsctr.o
MAINS = disksetup prtvtoc edvtoc diskformat

all : diskrm diskadd $(MAINS)
	echo "**** disksetup build completes" > /dev/null

install: all
	$(INS) -f $(USRSBIN) -m 0544 -u bin -g bin disksetup
	$(INS) -f $(USRSBIN) -m 0544 -u bin -g bin prtvtoc
	$(INS) -f $(USRSBIN) -m 0544 -u bin -g bin edvtoc
	$(INS) -f $(SBIN) -m 0755 -u root -g sys diskadd
	$(INS) -f $(SBIN) -m 0755 -u root -g sys diskrm
	$(INS) -f $(USRSBIN) -m 0755 -u root -g sys diskformat
	-mkdir ./tmp
	-$(CP) bfs.dflt ./tmp/bfs
	-$(CP) fstyp.dflt ./tmp/fstyp
	-$(CP) s5.dflt ./tmp/s5
	-$(CP) sfs.dflt ./tmp/sfs
	-$(CP) ufs.dflt ./tmp/ufs
	-$(CP) vxfs.dflt ./tmp/vxfs
	$(INS) -f $(ETC)/default -m 444 -u root -g sys ./tmp/bfs
	$(INS) -f $(ETC)/default -m 444 -u root -g sys ./tmp/fstyp
	$(INS) -f $(ETC)/default -m 444 -u root -g sys ./tmp/s5
	$(INS) -f $(ETC)/default -m 444 -u root -g sys ./tmp/sfs
	$(INS) -f $(ETC)/default -m 444 -u root -g sys ./tmp/ufs
	$(INS) -f $(ETC)/default -m 444 -u root -g sys ./tmp/vxfs
	-rm -rf ./tmp


disksetup:	 disksetup.o diskinit.o boot.o $(OFILES) 
	$(CC) -o disksetup disksetup.o diskinit.o boot.o $(OFILES) $(LDFLAGS) -lelf -lcmd


diskformat:	 diskformat.o $(OFILES) 
	$(CC) -o diskformat diskformat.o $(OFILES) $(LDFLAGS) -lelf

disksetup.o:	$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/ctype.h \
		$(INC)/malloc.h \
		$(INC)/string.h \
		$(INC)/sys/badsec.h \
		$(INC)/sys/types.h \
		$(INC)/sys/vtoc.h \
		$(INC)/sys/termios.h \
		$(INC)/sys/alttbl.h \
		$(INC)/sys/altsctr.h \
		$(INC)/sys/param.h \
		$(INC)/sys/fdisk.h \
		$(INC)/sys/fsid.h \
		$(INC)/sys/fstyp.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/swap.h \
		$(INC)/signal.h 

script.o:	

scsicomm.o:	

scl.o:	

scsi_setup.o:	

ix_altsctr.o:	$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/ctype.h \
		$(INC)/malloc.h \
		$(INC)/string.h \
		$(INC)/sys/badsec.h \
		$(INC)/sys/types.h \
		$(INC)/sys/vtoc.h \
		$(INC)/sys/alttbl.h \
		$(INC)/sys/altsctr.h \
		$(INC)/sys/param.h \
		$(INC)/sys/fdisk.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/swap.h \
		$(INC)/signal.h 

diskinit.o:	diskinit.c

boot.o:		boot.c

prtvtoc: prtvtoc.o ix_altsctr.o
	$(CC) -o prtvtoc prtvtoc.o ix_altsctr.o $(LDFLAGS)
	
edvtoc: edvtoc.o
	$(CC) -o edvtoc edvtoc.o $(LDFLAGS)

clean:
	rm -f *.o

clobber: clean
	rm -f $(MAINS) diskadd diskrm

strip: ALL
	$(STRIP) $(MAINS)
