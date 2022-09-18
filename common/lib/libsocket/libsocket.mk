#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libsocket:common/lib/libsocket/libsocket.mk	1.16.12.5"
#ident "$Header: libsocket.mk 1.5 91/06/27 $"

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
# 	(c) 1983,1984,1985,1986,1987,1988,1989,1990 AT&T
#	(c) 1990,1991 UNIX System Laboratories, Inc.
# 	          All rights reserved.
#  

include $(LIBRULES)

LOCALDEF= $(PICFLAG) -DSYSV
LDFLAGS=-G -dy -ztext -h /usr/lib/libsocket.so
ARFLAGS=crv
OWN=root
GRP=bin

INETOBJS=	bindresvport.o byteorder.o ether_addr.o getnetbyaddr.o \
		getnetbyname.o getnetent.o getproto.o getprotoent.o \
		getprotoname.o getservent.o gtservbyname.o gtservbyport.o \
		inet_addr.o inet_lnaof.o inet_mkaddr.o inet_netof.o \
		inet_network.o rcmd.o rexec.o ruserpass.o inet_sethost.o nd_gethost.o

SOCKOBJS=	accept.o bind.o callselect.o connect.o socket.o socketpair.o \
		shutdown.o getsockopt.o setsockopt.o listen.o \
		receive.o send.o _conn_util.o _utility.o \
		getsocknm.o getpeernm.o setsocknm.o setpeernm.o s_ioctl.o

LIBOBJS=	$(INETOBJS) $(SOCKOBJS)
ARNAME=		libsocket.a
LIBNAME=	libsocket.so

all:
		@for i in inet resolver socket;\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk all";\
			$(MAKE) -f $$i.mk all $(MAKEARGS) ; \
			cd ..;\
		done;\
		wait
		rm -f $(ARNAME) $(LIBNAME)
		$(AR) $(ARFLAGS) $(ARNAME) `$(LORDER) $(LIBOBJS) | $(TSORT)`
		if [ x$(CCSTYPE) != xCOFF ] ; \
		then \
		$(CC) $(LDFLAGS) -o $(LIBNAME) $(LIBOBJS) ;\
		fi
		-rm -f $(LIBOBJS)

install:	all
		cd resolver;\
		/bin/echo "\n===== $(MAKE) -f resolver.mk install";\
		$(MAKE) -f resolver.mk install $(MAKEARGS) ; \
		cd ..;\
		wait
		if [ x$(CCSTYPE) != xCOFF ] ; \
		then \
		$(INS) -f $(USRLIB) -m 0444 -u $(OWN) -g $(GRP) $(LIBNAME) ;\
		fi
		$(INS) -f $(USRLIB) -m 0444 -u $(OWN) -g $(GRP) $(ARNAME)

clean:
		@for i in inet resolver socket;\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk clean";\
			$(MAKE) -f $$i.mk clean $(MAKEARGS);\
			cd ..;\
		done;\
		wait
		-rm -f $(LIBOBJS)

clobber:	clean
		-rm -f $(LIBNAME) $(ARNAME)
