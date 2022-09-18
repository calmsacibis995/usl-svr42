#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dfs.cmds:lidload/lidload.mk	1.1.1.2"
#ident "$Header: lidload.mk 1.3 91/04/08 $"

include $(CMDRULES)

BINS= lidload
OBJS= lidload.o
SRCS= $(OBJS:.o=.c)
INSDIR = $(USRSBIN)
OWN = bin
GRP = bin
LOCALDEF=-DSYSV
LINTFLAGS= -bcnsu $(DEFLIST)
IS_COFF=`if [ x$(CCSTYPE) = xCOFF ] ; then echo "_s -lsocket" ; fi`
LDLIBS= -lnsl$(IS_COFF) -lcmd

all: $(BINS)


$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	if [ ! -d $(INSDIR) ] ; \
	then mkdir -p $(INSDIR); \
	fi
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(BINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

tags: $(SRCS)
	ctags -tw $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)

lidload.o: lidload.c \
	lidload.h
