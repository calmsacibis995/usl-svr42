#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)nfs.cmds:nfs/exportfs/exportfs.mk	1.11.5.3"
#ident	"$Header: $"

include $(CMDRULES)
INSDIR = $(USRSBIN)
OWN = bin
GRP = bin

all: exportfs.sh
	cp exportfs.sh exportfs && \
	chmod 0755 exportfs

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) exportfs

lintit:

tags:

clean:

clobber: clean
	-rm -f exportfs
