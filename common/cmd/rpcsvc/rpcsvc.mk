#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rpcsvc:rpcsvc.mk	1.25.9.2"
#ident  "$Header: rpcsvc.mk 1.5 91/09/20 $"

include $(CMDRULES)

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#       (c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc                     
#       (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.                      
#       (c) 1990,1991  UNIX System Laboratories, Inc
#          All rights reserved.
# 
LDLIBS = -lrpcsvc -lnsl
DESTSERVERS = $(USRSBIN)
DESTCLIENTS = $(USRBIN)
RWALL  = $(USRLIB)/netsvc/rwall
RUSERS = $(USRLIB)/netsvc/rusers
RSTAT  = $(USRLIB)/netsvc/rstat
SPRAY  = $(USRLIB)/netsvc/spray
TICLTS =$(ETC)/net/ticlts
TICOTS =$(ETC)/net/ticots
TICOTSORD =$(ETC)/net/ticotsord

DIRS =  $(RWALL) \
	$(RUSERS) \
	$(RSTAT) \
	$(SPRAY) \
	$(TICLTS) \
	$(TICOTS) \
	$(TICOTSORD) 

FILES= ./net_files/hosts ./net_files/services

ETCFILES= ./net_files/publickey \
	  ./net_files/netid \
	  ./net_files/rpc

SCLNTOBJS = spray.o spray_clnt.o
SSVCOBJS  = spray_subr.o spray_svc.o
ROBJS     = rpc.rusersd.o
WSVCOBJS  = rwall_subr.o rwall_svc.o
WCLNTOBJS = rwall.o rwall_clnt.o
OBJECTS   = $(SSVCOBJS) $(SCLNTOBJS) $(ROBJS) $(WCLNTOBJS) $(WSVCOBJS)
SOURCES   = $(OBJECTS:.o=.c)

SERVERS = rpc.rwalld rpc.sprayd rpc.rusersd rpc.rstatd
CLIENTS = rusers rup rwall spray
GOAL = $(SERVERS) $(CLIENTS) $(FILES)

.MUTEX: $(DIRS)

all: $(GOAL)

rup: rup.o rstatxdr.o
	$(CC) -o $@ rup.o rstatxdr.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rusers: rusers.o
	$(CC) -o $@ rusers.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rpc.rusersd: rpc.rusersd.o 
	$(CC) -o $@ rpc.rusersd.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rpc.rstatd: rpc.rstatd.o rstatxdr.o
	$(CC) -o $@ rpc.rstatd.o rstatxdr.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(LIBELF)

spray: $(SCLNTOBJS)
	$(CC) -o $@ $(SCLNTOBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rpc.sprayd: $(SSVCOBJS)
	$(CC) -o $@ $(SSVCOBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rwall: $(WCLNTOBJS)
	$(CC) -o $@ $(WCLNTOBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rpc.rwalld: $(WSVCOBJS)
	$(CC) -o $@ $(WSVCOBJS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

domainname.o: domainname.c \
	$(INC)/stdio.h

rpc.rusersd.o: rpc.rusersd.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/rpc/rpc.h \
	$(INC)/memory.h \
	$(INC)/netconfig.h \
	$(INC)/stropts.h \
	$(INC)/syslog.h \
	$(INC)/utmp.h \
	$(INC)/rpcsvc/rusers.h \
	$(INC)/sys/resource.h

rusers.o: rusers.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpcsvc/rusers.h \
	$(INC)/string.h

rwall.o: rwall.c \
	$(INC)/stdio.h \
	$(INC)/utmp.h \
	$(INC)/rpc/rpc.h \
	$(INC)/signal.h \
	$(INC)/setjmp.h \
	$(INC)/rpcsvc/rwall.h

rwall_clnt.o: rwall_clnt.c \
	$(INC)/rpc/rpc.h \
	$(INC)/rpcsvc/rwall.h

rwall_subr.o: rwall_subr.c \
	$(INC)/rpc/rpc.h \
	$(INC)/stdio.h \
	$(INC)/rpcsvc/rwall.h

rwall_svc.o: rwall_svc.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/stat.h \
	$(INC)/memory.h \
	$(INC)/netconfig.h \
	$(INC)/stropts.h \
	$(INC)/syslog.h \
	$(INC)/rpcsvc/rwall.h \
	$(INC)/sys/resource.h

spray.o: spray.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpcsvc/spray.h

spray_clnt.o: spray_clnt.c \
	$(INC)/rpc/rpc.h \
	$(INC)/rpcsvc/spray.h

spray_subr.o: spray_subr.c \
	$(INC)/rpc/rpc.h \
	$(INC)/rpcsvc/spray.h

spray_svc.o: spray_svc.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/stat.h \
	$(INC)/memory.h \
	$(INC)/netconfig.h \
	$(INC)/stropts.h \
	$(INC)/syslog.h \
	$(INC)/rpcsvc/spray.h \
	$(INC)/sys/resource.h

$(DIRS):
	- [ -d $@ ] || mkdir -p $@ ;\
		$(CH)chmod 755 $@ ;\
		$(CH)chgrp sys $@ ;\
		$(CH)chown root $@

install: $(GOAL) $(DIRS)
	$(INS) -f $(RWALL) -m 0555 -u bin -g bin rpc.rwalld
	$(INS) -f $(RUSERS) -m 0555 -u bin -g bin rpc.rusersd
	$(INS) -f $(RSTAT) -m 0555 -u bin -g bin rpc.rstatd
	$(INS) -f $(SPRAY) -m 0555 -u bin -g bin rpc.sprayd
	$(INS) -f $(DESTCLIENTS) -m 0555 -u bin -g bin rusers
	$(INS) -f $(DESTSERVERS) -m 0555 -u bin -g bin rwall
	$(INS) -f $(DESTSERVERS) -m 0555 -u bin -g bin spray
	for i in $(FILES) ; \
	do \
		$(INS) -f $(TICLTS) -m 0444 -u root -g sys $$i ; \
		$(INS) -f $(TICOTS) -m 0444 -u root -g sys $$i ; \
		$(INS) -f $(TICOTSORD) -m 0444 -u root -g sys $$i ; \
	done 
	for i in $(ETCFILES) ; \
	do \
		$(INS) -f $(ETC) -m 0644 -u root -g sys $$i ; \
	done

lintit:
	$(LINT) $(LINTFLAGS) spray.c spray_clnt.c
	$(LINT) $(LINTFLAGS) spray_subr.c spray_svc.c
	$(LINT) $(LINTFLAGS) rpc.rusersd.c
	$(LINT) $(LINTFLAGS) rpc.rstatd.c rstatxdr.c
	$(LINT) $(LINTFLAGS) rwall_subr.c rwall_svc.c
	$(LINT) $(LINTFLAGS) rwall.c rwall_clnt.c

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(CLIENTS) $(SERVERS)
