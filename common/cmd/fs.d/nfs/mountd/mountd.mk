#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)nfs.cmds:nfs/mountd/mountd.mk	1.17.4.2"
#ident	"$Header: $"

include $(CMDRULES)

BINS= mountd
OBJS= rpc.mountd.o issubdir.o mountxdr.o innetgr.o sharetab.o 
SRCS= $(OBJS:.o=.c)
INCSYS = $(INC)
INSDIR = $(USRLIB)/nfs
OWN = bin
GRP = bin

LINTFLAGS= -hbax $(DEFLIST)
COFFLIBS= -lrpc -ldes -lnsl_s -lyp -lnet -lsocket
#ELFLIBS = -dy -lnsl -lyp -lrpc -ldes -lnet -lsocket
ELFLIBS = -lnsl -lsocket
LDLIBS=`if [ x$(CCSTYPE) = xCOFF ] ; then echo "$(COFFLIBS)" ; else echo "$(ELFLIBS)" ; fi`

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(BINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

tags: $(SRCS)
	ctags $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)
