#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:ufs/tunefs/tunefs.mk	1.5.6.2"
#ident "$Header: tunefs.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
INSDIR2 = $(ETC)/fs/ufs
OWN = bin
GRP = bin

OBJS=

all:  tunefs

tunefs: tunefs.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: tunefs
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) tunefs
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) tunefs
	
clean:
	-rm -f tunefs.o

clobber: clean
	rm -f tunefs
