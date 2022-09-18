#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nfs.cmds:nfs/lockd/lockd.mk	1.11.8.3"
#ident	"$Header: $"

include $(CMDRULES)

OWN = bin
GRP = bin

BINS= lockd
OBJS= xdr_klm.o xdr_nlm.o xdr_sm.o prot_pklm.o prot_msg.o prot_lock.o \
      prot_alloc.o prot_free.o prot_share.o hash.o ufs_lockf.o \
      prot_proc.o prot_libr.o signal.o prot_pnlm.o \
      prot_priv.o  sm_monitor.o prot_main.o setbuffer.o \
      rpc.o svc_create.o flk_reclox.o

# pmap_clnt.o pmap_prot.o rpc_soc.o svc.o ti_opts.o svc_dg.o

SRCS= $(OBJS:.o=.c)
INCSYS = $(INC)
INSDIR = $(USRLIB)/nfs

LINTFLAGS= -hbax $(DEFLIST)
COFFLIBS= -lrpc -ldes -lnsl_s -lsocket
#ELFLIBS = -lrpcsvc -dy -lnsl -lrpc -ldes -lnet -lsocket
ELFLIBS = -L /usr/ucblib -dy -lnsl -lsocket
LDLIBS = `if [ x$(CCSTYPE) = xCOFF ] ; then echo "$(COFFLIBS)" ; else echo "$(ELFLIBS)" ; fi`

all: $(BINS)

$(BINS): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(SHLIBS) $(LDLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(BINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

tags: $(SRCS)
	ctags -tw $(SRCS)

clean:
	-rm -f $(OBJS)

clobber: clean
	-rm -f $(BINS)
