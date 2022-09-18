#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:ufs/ufsrestore/ufsrestore.mk	1.10.6.2"
#ident "$Header: ufsrestore.mk 1.3 91/04/17 $"

include $(CMDRULES)

# LOCALDEF:
#       DEBUG                   use local directory to find ddate and dumpdates
#       TDEBUG                  trace out the process forking
#
BINS= ufsrestore
OBJS= dirs.o interactive.o main.o restore.o symtab.o \
	tape.o utilities.o
SRCS= dirs.c interactive.c main.c restore.c symtab.c \
	tape.c utilities.c
HDRS= dump.h

INSDIR1 = $(USRLIB)/fs/ufs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin

RM= rm -f

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(SHLIBS)

install: $(BINS)
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) ufsrestore
	-rm -f $(INSDIR2)/ufsrestore
	ln $(INSDIR1)/ufsrestore $(INSDIR2)/ufsrestore
	
clean:
	$(RM) $(BINS) $(OBJS)

clobber: clean
	$(RM) ufsrestore
