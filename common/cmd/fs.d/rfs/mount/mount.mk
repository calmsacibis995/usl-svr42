#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfs.cmds:rfs/mount/mount.mk	1.15.5.4"
#ident	"$Header: mount.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR = $(ETC)/fs/rfs
OWN = bin
GRP = bin
INCSYS = $(INC)
LOCALINC=-I.
LINTFLAGS= $(DEFLIST) -ux

COFFLIBS= -lns -lnsl_s -lsocket
ELFLIBS = -lns -lnsl
LDLIBS=`if [ x$(CCSTYPE) = xCOFF ] ; then echo "$(COFFLIBS)" ; else echo "$(ELFLIBS)" ; fi`
FRC =

all: mount

mount: mount.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

mount.o: mount.c \
	$(INCSYS)/sys/types.h \
	$(INC)/nserve.h \
	$(INC)/netconfig.h \
	$(INCSYS)/sys/stropts.h \
	$(INCSYS)/sys/rf_cirmgr.h \
	$(INCSYS)/sys/vfs.h \
	$(INCSYS)/sys/vnode.h \
	$(INCSYS)/sys/fs/rf_vfs.h \
	$(INCSYS)/sys/rf_sys.h \
	$(INCSYS)/sys/list.h \
	$(INCSYS)/sys/rf_messg.h \
	$(INCSYS)/sys/rf_comm.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INCSYS)/sys/mnttab.h \
	$(INC)/unistd.h \
	$(INC)/errno.h \
	$(INC)/pn.h \
	$(INCSYS)/sys/mount.h \
	$(INCSYS)/sys/conf.h \
	$(FRC)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	[ -d $(USRLIB)/fs/rfs ] || mkdir -p $(USRLIB)/fs/rfs
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(USRLIB)/fs/rfs -m 0555 -u $(OWN) -g $(GRP) mount

clean:
	rm -f *.o

clobber: clean
	rm -f mount

lintit:
	$(LINT) $(LINTFLAGS) mount.c
FRC:
