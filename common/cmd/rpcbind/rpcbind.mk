#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rpcbind:rpcbind.mk	1.13.10.3"
#ident  "$Header: rpcbind.mk 1.4 91/06/28 $"

include $(CMDRULES)


OWN = bin
GRP = bin

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
#	(c) 1986,1987,1988.1989,1990  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#	(c) 1990,1991  UNIX System Laboratories, Inc.
#          All rights reserved.
# 
#
# Makefile for rpcbind
#

LDLIBS = -lnsl
# LOCALDEF = -DBIND_DEBUG
#LOCALDEF = -DPORTMAP
CFLAGS= -DPORTMAP

OBJECTS = rpcbind.o rpcb_svc.o pmap_svc.o check_bound.o stricmp.o warmstart.o

SOURCES = $(OBJECTS:.o=.c)

all: $(OBJECTS)
	$(CC) -DPORTMAP -o rpcbind $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS) 

check_bound.o: check_bound.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h \
	$(INC)/sys/syslog.h

pmap_svc.o: pmap_svc.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/pmap_prot.h \
	$(INC)/rpc/rpcb_prot.h \
	rpcbind.h \
	$(INC)/netdb.h \
	$(INC)/netinet/in.h \
	$(INC)/sys/ioctl.h \
	$(INC)/sys/socket.h

rpcb_svc.o: rpcb_svc.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/rpcb_prot.h \
	$(INC)/netconfig.h \
	$(INC)/sys/param.h \
	$(INC)/sys/errno.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/pmap_prot.h \
	$(INC)/sys/syslog.h \
	$(INC)/netdir.h \
	rpcbind.h

rpcbind.o: rpcbind.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/unistd.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/rpcb_prot.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/signal.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/pmap_prot.h \
	$(INC)/sys/termios.h \
	rpcbind.h \
	$(INC)/sys/syslog.h

stricmp.o: stricmp.c

warmstart.o: warmstart.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/rpcb_prot.h \
	$(INC)/sys/stat.h \
	$(INC)/netinet/in.h \
	$(INC)/rpc/pmap_prot.h \
	rpcbind.h \
	$(INC)/sys/syslog.h

install: all
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) rpcbind

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES) 

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f rpcbind
