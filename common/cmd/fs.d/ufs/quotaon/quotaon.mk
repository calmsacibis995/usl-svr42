#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:ufs/quotaon/quotaon.mk	1.7.5.2"
#ident "$Header: quotaon.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
INSDIR2 = $(USRSBIN)
OWN =  bin
GRP = bin
OBJS=

all:  quotaon

quotaon: quotaon.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@  $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: quotaon
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/quotaon
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) quotaon
	-rm -f $(INSDIR)/quotaoff
	ln $(INSDIR)/quotaon $(INSDIR)/quotaoff
	-rm -f $(INSDIR2)/quotaon
	ln $(INSDIR)/quotaon $(INSDIR2)/quotaon
	-rm -f $(INSDIR2)/quotaoff
	ln $(INSDIR)/quotaon $(INSDIR2)/quotaoff
	
clean:
	-rm -f quotaon.o

clobber: clean
	rm -f quotaon
