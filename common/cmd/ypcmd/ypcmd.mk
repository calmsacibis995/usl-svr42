#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ypcmd:ypcmd.mk	1.6.16.4"
#ident  "$Header: ypcmd.mk 1.9 91/09/20 $"

include $(CMDRULES)

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
# Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
# publication.
#
#     (c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
#     (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#     (c) 1990,1991  UNIX System Laboratories, Inc.
#     All rights reserved.
# 

LDFLAGS = -s -dy
LDLIBS  = -lnsl

BIN = ypmatch ypwhich ypcat 
SBIN = makedbm yppoll yppush ypset ypxfr ypalias 
SCRIPTS = ypinit ypxfr_1day ypxfr_2day ypxfr_1hour
YPSVC = ypbind ypserv ypupdated udpublickey

YPSERVOBJ = ypserv.o ypserv_ancil.o ypserv_map.o ypserv_proc.o \
	yp_getalias.o getlist.o
YPBINDOBJ = yp_b_svc.o yp_b_subr.o pong.o getlist.o yp_getalias.o
YPUPDOBJ = ypupdated.o openchild.o

NETSVC = $(USRLIB)/netsvc
NETYP  = $(NETSVC)/yp
YP = $(VAR)/yp
BINDINGS= $(YP)/binding

VARFILES= ./net_files/aliases ./net_files/updaters ./net_files/Makefile


all: $(BIN) $(SBIN) $(SCRIPTS) $(YPSVC) $(ETCFILES) $(VARFILES)

ypserv: $(YPSERVOBJ)
	$(CC) -o $@ $(YPSERVOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	
ypbind: $(YPBINDOBJ)
	$(CC) -o $@ $(YPBINDOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ypupdated: $(YPUPDOBJ)
	$(CC) -o $@ $(YPUPDOBJ) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

udpublickey: udpublickey.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)
makedbm yppush: $$@.o yp_getalias.o getlist.o
	$(CC) -o $@ $@.o yp_getalias.o getlist.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ypxfr: $$@.o ypalias.o yp_getalias.o getlist.o 
	$(CC) -o $@ $@.o ypalias.o yp_getalias.o getlist.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ypalias: ypaliasm.o yp_getalias.o getlist.o
	$(CC) -o $@ ypaliasm.o yp_getalias.o getlist.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(BIN) yppoll ypset: $$@.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

getlist.o: getlist.c \
	$(INC)/stdio.h 

makedbm.o: makedbm.c \
	$(INC)/rpcsvc/dbm.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/file.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/ctype.h \
	ypdefs.h 

openchild.o: openchild.c \
	$(INC)/stdio.h 

pong.o: pong.c \
	$(INC)/rpc/rpc.h \
	yp_b.h \
	$(INC)/rpcsvc/yp_prot.h 

yp_b_subr.o: yp_b_subr.c \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpcsvc/yp_prot.h \
	$(INC)/dirent.h \
	$(INC)/sys/wait.h \
	yp_b.h 

yp_b_svc.o: yp_b_svc.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/nettype.h \
	yp_b.h \
	$(INC)/netconfig.h 

ypcat.o: ypcat.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	yp_b.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/rpcsvc/yp_prot.h 

ypmatch.o: ypmatch.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/socket.h \
	$(INC)/rpcsvc/yp_prot.h \
	$(INC)/rpcsvc/ypclnt.h 

yppoll.o: yppoll.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/socket.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/rpcsvc/yp_prot.h \
	yp_b.h 

yppush.o: yppush.c \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/signal.h \
	$(INC)/sys/types.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/stat.h \
	$(INC)/ctype.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/rpcb_prot.h \
	$(INC)/rpc/rpcb_clnt.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/rpcsvc/yp_prot.h \
	yp_b.h \
	ypdefs.h

ypserv.o: ypserv.c \
	ypsym.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/file.h \
	ypdefs.h 

ypserv_ancil.o: ypserv_ancil.c \
	ypsym.h \
	ypdefs.h \
	$(INC)/dirent.h 

ypserv_map.o: ypserv_map.c \
	ypsym.h \
	ypdefs.h \
	$(INC)/dirent.h \
	$(INC)/ctype.h 

ypserv_proc.o: ypserv_proc.c \
	ypsym.h \
	ypdefs.h \
	$(INC)/ctype.h 

ypset.o: ypset.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpcsvc/ypclnt.h \
	yp_b.h \
	$(INC)/rpcsvc/yp_prot.h 

ypupdated.o: ypupdated.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/file.h \
	$(INC)/sys/wait.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/auth_des.h \
	$(INC)/rpc/nettype.h \
	$(INC)/rpcsvc/ypupd.h \
	$(INC)/rpcsvc/ypclnt.h 

ypwhich.o: ypwhich.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/rpc/rpc.h \
	yp_b.h \
	$(INC)/rpcsvc/yp_prot.h \
	ypv2_bind.h \
	$(INC)/rpcsvc/ypclnt.h 

ypxfr.o: ypxfr.c \
	$(INC)/rpcsvc/dbm.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/ctype.h \
	$(INC)/dirent.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/file.h \
	$(INC)/sys/stat.h \
	$(INC)/rpcsvc/ypclnt.h \
	ypdefs.h \
	$(INC)/rpcsvc/yp_prot.h \
	yp_b.h

udpublickey.o: udpublickey.c

ypalias.o: ypalias.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/limits.h \
	$(INC)/sys/types.h \
	$(INC)/sys/statvfs.h \
	ypsym.h

ypaliasm.o: ypaliasm.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/limits.h \
	$(INC)/sys/types.h \
	$(INC)/sys/statvfs.h \
	ypsym.h
	$(CC) $(CFLAGS) $(DEFLIST) -DMAIN -c ypaliasm.c

ypaliasm.c:
	-$(CP) ypalias.c ypaliasm.c

install: all
	[ -d $(NETSVC) ] || mkdir -p $(NETSVC)
	[ -d $(NETYP) ] || mkdir -p $(NETYP)
	$(INS) -f $(USRBIN) ypmatch
	$(INS) -f $(USRBIN) ypcat
	$(INS) -f $(USRBIN) ypwhich
	$(INS) -f $(NETYP) ypserv
	$(INS) -f $(USRSBIN) ypalias
	$(INS) -f $(USRSBIN) makedbm 
	$(INS) -f $(USRSBIN) yppoll
	$(INS) -f $(USRSBIN) yppush
	$(INS) -f $(USRSBIN) ypset
	$(INS) -f $(USRSBIN) ypxfr
	$(INS) -f $(USRSBIN) ypinit
	$(INS) -f $(NETYP) ypbind 
	$(INS) -f $(NETYP) ypupdated
	$(INS) -f $(USRSBIN) udpublickey
	[ -d $(YP) ] || mkdir -p $(YP)
	[ -d $(BINDINGS) ] || mkdir -p $(BINDINGS)
	for i in $(VARFILES) ; \
	do \
		$(INS) -f $(YP) -m 0444 -u root -g sys $$i ; \
	done 
	cp $(USRCCS)/bin/make $(YP)/ypbuild

clean: 
	-rm -f getlist.o makedbm.o openchild.o \
		$(YPSERVOBJ) $(YPBINDOBJ) ypcat.o ypmatch.o yppoll.o \
		ypupdated.o ypwhich.o ypxfr.o yppush.o ypset.o ypaliasm.c \
		udpublickey.o

clobber: clean
	-rm -f $(BIN) $(SBIN) $(SCRIPTS) $(YPSVC)

lintit:
	$(LINT) $(LINTFLAGS) $(YPSERVOBJ:.o=.c) 
	$(LINT) $(LINTFLAGS) $(YPBINDOBJ:.o=.c)
	$(LINT) $(LINTFLAGS) $(YPUPDOBJ:.o=.c)
	$(LINT) $(LINTFLAGS) ypmatch.c
	$(LINT) $(LINTFLAGS) ypwhich.c
	$(LINT) $(LINTFLAGS) ypcat.c
	$(LINT) $(LINTFLAGS) yppoll.c
	$(LINT) $(LINTFLAGS) yppush.c
	$(LINT) $(LINTFLAGS) ypset.c
	$(LINT) $(LINTFLAGS) ypxfr.c yp_getalias.c getlist.c
	$(LINT) $(LINTFLAGS) makedbm.c yp_getalias.c getlist.c

