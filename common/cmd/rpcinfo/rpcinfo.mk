#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rpcinfo:rpcinfo.mk	1.10.9.2"
#ident  "$Header: rpcinfo.mk 1.4 91/07/01 $"

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
#       (c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc                     
#       (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.                      
#       (c) 1990,1991  UNIX System Laboratories, Inc
#          All rights reserved.
# 
#
# Makefile for rpcinfo
#

LDLIBS= -lnsl
LOCALDEF= -DPORTMAP
MAIN = rpcinfo

OBJECTS = rpcinfo.o
SRCS = $(OBJECTS:.o=.c)

all: $(MAIN)

$(MAIN): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS) 

rpcinfo.o: rpcinfo.c \
	$(INC)/rpc/rpc.h \
	$(INC)/stdio.h \
	$(INC)/rpc/rpcb_prot.h \
	$(INC)/rpc/nettype.h \
	$(INC)/netdir.h \
	$(INC)/rpc/rpcent.h \
	$(INC)/netinet/in.h \
	$(INC)/sys/socket.h \
	$(INC)/sys/utsname.h \
	$(INC)/netdb.h \
	$(INC)/arpa/inet.h \
	$(INC)/rpc/pmap_prot.h \
	$(INC)/rpc/pmap_clnt.h

install: $(MAIN)
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) $(MAIN)

lintit:
	$(LINT) $(LINTFLAGS) $(CPPFLAGS) $(SRCS) 

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAIN)
