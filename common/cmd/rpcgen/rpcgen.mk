#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rpcgen:rpcgen.mk	1.10.10.3"
#ident  "$Header: rpcgen.mk 1.4 91/07/01 $"

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
#	Makefile 1.14 89/03/30 (C) 1987 SMI
#
# Makefile for rpc protocol compiler
# Copyright (C) 1987, Sun Microsystems, Inc.
#

OBJECTS = rpc_clntout.o rpc_cout.o rpc_hout.o rpc_main.o rpc_parse.o \
	  rpc_scan.o rpc_svcout.o rpc_tblout.o rpc_util.o
SOURCES = $(OBJECTS:.o=.c)

all: rpcgen

rpcgen: $(OBJECTS)
	$(CC) -o rpcgen $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rpc_clntout.o: rpc_clntout.c \
	rpc_parse.h \
	rpc_util.h \
	$(INC)/stdio.h

rpc_cout.o: rpc_cout.c \
	rpc_util.h \
	$(INC)/stdio.h \
	rpc_parse.h

rpc_hout.o: rpc_hout.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	rpc_util.h \
	rpc_parse.h

rpc_main.o: rpc_main.c \
	rpc_util.h \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/file.h \
	rpc_parse.h \
	rpc_scan.h

rpc_parse.o: rpc_parse.c \
	$(INC)/stdio.h \
	rpc_util.h \
	rpc_scan.h \
	rpc_parse.h

rpc_scan.o: rpc_scan.c \
	rpc_scan.h \
	rpc_util.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h

rpc_svcout.o: rpc_svcout.c \
	rpc_parse.h \
	rpc_util.h \
	$(INC)/stdio.h

rpc_tblout.o: rpc_tblout.c \
	rpc_parse.h \
	rpc_util.h \
	$(INC)/stdio.h

rpc_util.o: rpc_util.c \
	rpc_scan.h \
	rpc_parse.h \
	rpc_util.h \
	$(INC)/stdio.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f rpcgen

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

install: all
	[ -d $(USRBIN) ] || mkdir $(USRBIN)
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) rpcgen
