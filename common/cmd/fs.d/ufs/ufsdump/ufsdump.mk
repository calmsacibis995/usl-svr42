#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:ufs/ufsdump/ufsdump.mk	1.12.6.2"
#ident "$Header: ufsdump.mk 1.2 91/04/11 $"

include $(CMDRULES)

#       dump.h                  header file
#       dumpitime.c             reads /etc/dumpdates
#       dumpmain.c              driver
#       dumpoptr.c              operator interface
#       dumptape.c              handles the mag tape and opening/closing
#       dumptraverse.c          traverses the file system
#       unctime.c               undo ctime
#
# LOCALDEF:
#       DEBUG                   use local directory to find ddate and dumpdates
#       TDEBUG                  trace out the process forking
#
BINS= ufsdump
OBJS= dumpitime.o dumpmain.o dumpoptr.o dumptape.o \
	dumptraverse.o unctime.o
SRCS= dumpitime.c dumpmain.c dumpoptr.c dumptape.c \
	dumptraverse.c unctime.c
HDRS= dump.h

INSDIR1 = $(USRLIB)/fs/ufs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(SHLIBS)

install: $(BINS)
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) ufsdump
	-rm -f $(INSDIR2)/ufsdump
	ln $(INSDIR1)/ufsdump $(INSDIR2)/ufsdump
	
clean:
	rm -f $(BINS) $(OBJS)

clobber: clean
	rm -f $(BINS)
