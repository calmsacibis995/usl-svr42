#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/resolv/resolv.mk	1.3.6.6"
#ident "$Header: resolv.mk 1.3 91/03/15 $"

#	Makefile for resolv.so

include $(LIBRULES)

LIBNAME=	resolv.so

RESOLVOBJS=	gthostnamadr.o res_comp.o res_debug.o res_init.o \
		res_mkquery.o res_query.o res_send.o sethostent.o \
		strcasecmp.o
INETOBJS=	bindresvport.o byteorder.o ether_addr.o getnetbyaddr.o \
		getnetbyname.o getnetent.o getproto.o getprotoent.o \
		getprotoname.o getservent.o gtservbyname.o gtservbyport.o \
		inet_addr.o inet_lnaof.o inet_mkaddr.o inet_netof.o \
		inet_network.o rcmd.o rexec.o ruserpass.o
SOCKOBJS=	accept.o bind.o connect.o socket.o socketpair.o \
		shutdown.o getsockopt.o setsockopt.o listen.o \
		receive.o send.o _conn_util.o _utility.o \
		getsocknm.o getpeernm.o setsocknm.o setpeernm.o s_ioctl.o
OBJS=		resolv.o $(RESOLVOBJS) $(INETOBJS) $(SOCKOBJS)

LOCALDEF=-DSYSV -D_RESOLV_ABI $(PICFLAG)
LOCALLDFLAGS=-dy -G -ztext -h /usr/lib/$(LIBNAME)

all:		resolv.o
		@for i in libresolv libsocket;\
		do\
			cd $$i;\
			if [ x$(CCSTYPE) = xCOFF ] ; \
			then \
			echo "\n===== $(MAKE) -f $$i.mk all";\
			$(MAKE) -f $$i.mk all CFLAGS=-O $(MAKEARGS) \
				LOCALDEF="$(LOCALDEF)";\
			else \
			echo "\n===== $(MAKE) -f $$i.mk all";\
			$(MAKE) -f $$i.mk all $(MAKEARGS) \
				LOCALDEF="$(LOCALDEF)";\
			fi ;\
			cd ..;\
		done;\
		wait
		$(CC) $(LOCALLDFLAGS) -o $(LIBNAME) $(OBJS)
		rm -f $(OBJS)

clean:
		@for i in libresolv libsocket;\
		do\
			cd $$i;\
			echo "\n===== $(MAKE) -f $$i.mk clean";\
			$(MAKE) -f $$i.mk clean $(MAKEARGS);\
			cd ..;\
		done;\
		wait
		rm -f $(OBJS)

clobber:
		@for i in libresolv libsocket;\
		do\
			cd $$i;\
			echo "\n===== $(MAKE) -f $$i.mk clobber";\
			$(MAKE) -f $$i.mk clobber $(MAKEARGS);\
			cd ..;\
		done;\
		wait
		rm -f $(OBJS) $(LIBNAME)

#
#	The link of libresolv.so to resolv.so is for
#	V4 compat...

install:	all
		$(INS) -f $(USRLIB) $(LIBNAME)
		rm -f $(USRLIB)/libresolv.so
		ln $(USRLIB)/$(LIBNAME) $(USRLIB)/libresolv.so

size:		all
		$(SIZE) $(LIBNAME)

strip:		all
		$(STRIP) $(LIBNAME)

lintit:

#
# header dependencies
#
resolv.o:	resolv.c \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/sockio.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/tiuser.h \
		$(INC)/netconfig.h \
		$(INC)/netdir.h \
		$(INC)/string.h \
		$(INC)/fcntl.h \
		$(INC)/sys/param.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/utsname.h \
		$(INC)/net/if.h \
		$(INC)/stropts.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/syslog.h \
		$(FRC)
