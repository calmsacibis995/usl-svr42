#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:ufs/repquota/repquota.mk	1.7.6.2"
#ident "$Header: repquota.mk 1.4 91/06/11 $"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
SINSDIR = /usr/lib/fs/ufs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin
OBJS=

all:  repquota

repquota: repquota.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: repquota
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) repquota
	-rm -f $(INSDIR2)/repquota
	ln $(INSDIR)/repquota $(INSDIR2)/repquota
	rm -f $(USRBIN)/repquota
	$(SYMLINK) $(SINSDIR)/repquota $(USRBIN)/repquota
	
clean:
	-rm -f repquota.o

clobber: clean
	rm -f repquota
