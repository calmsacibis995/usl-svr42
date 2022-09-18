#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.named/tools/tools.mk	1.14.9.1"
#ident  "$Header: tools.mk 1.3 91/06/26 $"

#
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# 		PROPRIETARY NOTICE (Combined)
# 
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
# 
# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
# 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#       (c) 1990,1991  UNIX System Laboratories, Inc.
# 	          All rights reserved.
#  
#

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP -DDEBUG
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lresolv -lsocket -lnsl

all:		nstest lookdir

install:	nstest
		$(INS) -f $(INSDIR) -m 0555 -u bin -g bin nstest
		@cd nslookup;\
		/bin/echo "\n===== $(MAKE) -f nslookup.mk install";\
		$(MAKE) -f nslookup.mk install $(MAKEARGS)

nstest:		nstest.o
		$(CC) -o nstest $(LDFLAGS) nstest.o $(LDLIBS) $(SHLIBS)

lookdir:
		@cd nslookup;\
		/bin/echo "\n===== $(MAKE) -f nslookup.mk all";\
		$(MAKE) -f nslookup.mk all $(MAKEARGS)

clean:
		rm -f *.o
		@cd nslookup;\
		/bin/echo "\n===== $(MAKE) -f nslookup.mk clean";\
		$(MAKE) -f nslookup.mk clean $(MAKEARGS)

clobber:
		rm -f *.o nstest
		@cd nslookup;\
		/bin/echo "\n===== $(MAKE) -f nslookup.mk clobber";\
		$(MAKE) -f nslookup.mk clobber $(MAKEARGS)

lintit:
		$(LINT) $(LINTFLAGS) *.c
		@cd nslookup;\
		/bin/echo "\n===== $(MAKE) -f nslookup.mk lintit";\
		$(MAKE) -f nslookup.mk lintit $(MAKEARGS)

FRC:

#
# Header dependencies
#

nstest.o:	nstest.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		$(FRC)
