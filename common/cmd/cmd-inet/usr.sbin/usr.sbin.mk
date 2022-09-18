#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/usr.sbin.mk	1.26.15.5"
#ident	"$Header: $"

# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#  

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP
LICENSE	=	-DLIMITED
INSDIR=		$(USRSBIN)
OWN=		root
GRP=		bin

YFLAGS = -d

LDLIBS=		-lsocket -lnsl -lcmd -lcrypt_i -lgen -lia -liaf $(LIBELF)

#	don't include 'arp' and 'ping' and 'scheme' in PRODUCTS list because
#	they have to be installed with set[ug]id on
#	or need libiaf.  They can not be made by default rules.
ALL=		arp ping scheme $(PRODUCTS)
PRODUCTS=	gettable ifconfig in.comsat in.fingerd in.rarpd \
		in.rexecd in.rlogind in.rshd in.rwhod in.telnetd in.tftpd \
		in.tnamed inetd route trpt
DIRS=		htable in.ftpd in.named in.routed in.talkd slink
IAFDIR=		$(USRLIB)/iaf
SCHEMEDIR=	$(IAFDIR)/in.login
.o:
		$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $< $(LDLIBS) $(SHLIBS)

all:		$(ALL)
		@for i in $(DIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk all";\
			$(MAKE) -f $$i.mk all $(MAKEARGS);\
			cd ..;\
		done;\
		wait

$(IAFDIR):
		-mkdir $(IAFDIR)
		$(CH)-chmod 755 $(IAFDIR)
		$(CH)-chgrp sys $(IAFDIR)
		$(CH)-chown $(OWN) $(IAFDIR)

$(SCHEMEDIR):	$(IAFDIR)
		-mkdir $(SCHEMEDIR)
		$(CH)-chmod 755 $(SCHEMEDIR)
		$(CH)-chgrp sys $(SCHEMEDIR)
		$(CH)-chown $(OWN) $(SCHEMEDIR)

scheme.install:	$(SCHEMEDIR) scheme
		$(INS) -f $(SCHEMEDIR) -m 04550 -u $(OWN) -g $(GRP) scheme

install:	$(ALL) scheme.install
		@for i in $(PRODUCTS);\
		do\
			$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $$i;\
		done
		$(INS) -f $(INSDIR) -m 02555 -u bin -g sys arp
		$(INS) -f $(INSDIR) -m 04755 -u $(OWN) -g $(GRP) ping
		@for i in $(DIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk install";\
			$(MAKE) -f $$i.mk install $(MAKEARGS);\
			cd ..;\
		done;\
		wait

#	these targets use default rule
gettable:	gettable.o
		$(CC) -o $@ $(LDFLAGS) gettable.o $(LDLIBS) $(SHLIBS)
ifconfig:	ifconfig.o
		$(CC) -o $@ $(LDFLAGS) ifconfig.o $(LDLIBS) $(SHLIBS)
in.comsat:	in.comsat.o security.o
		$(CC) -o $@ $(LDFLAGS) in.comsat.o security.o $(LDLIBS) $(SHLIBS)
in.fingerd:	in.fingerd.o
		$(CC) -o $@ $(LDFLAGS) in.fingerd.o $(LDLIBS) $(SHLIBS)
in.rarpd:	in.rarpd.o
		$(CC) -o $@ $(LDFLAGS) in.rarpd.o $(LDLIBS) $(SHLIBS)
in.rexecd:	in.rexecd.o security.o
		$(CC) -o $@ $(LDFLAGS) in.rexecd.o security.o $(LDLIBS) $(SHLIBS)
in.rlogind:	in.rlogind.o security.o
		$(CC) -o $@ $(LDFLAGS) in.rlogind.o security.o $(LDLIBS) $(SHLIBS)
in.rshd:	in.rshd.o security.o
		$(CC) -o $@ $(LDFLAGS) in.rshd.o security.o $(LDLIBS) $(SHLIBS)
in.telnetd:	in.telnetd.o security.o
		$(CC) -o $@ $(LDFLAGS) in.telnetd.o security.o $(LDLIBS) $(SHLIBS)
in.tnamed:	in.tnamed.o
		$(CC) -o $@ $(LDFLAGS) in.tnamed.o $(LDLIBS) $(SHLIBS)
inetd:		inetd.o security.o
		$(CC) -o $@ $(LDFLAGS) inetd.o security.o $(LDLIBS) $(SHLIBS)
ping:		ping.o
		$(CC) -o $@ $(LDFLAGS) ping.o $(LDLIBS) $(SHLIBS)

#	these targets can't use default rule -- need extra .o's or libs
arp:		arp.o
		$(CC) -o $@ $(LDFLAGS) arp.o $(LDLIBS) $(SHLIBS)

in.rwhod:	in.rwhod.o
		$(CC) -o $@ $(LDFLAGS) in.rwhod.o $(LDLIBS) $(SHLIBS)

in.tftpd:	in.tftpd.o tftpsubs.o security.o
		$(CC) $(LDFLAGS) -o $@ in.tftpd.o tftpsubs.o security.o $(LDLIBS) $(SHLIBS)

route:		route.o
		$(CC) -o $@ $(LDFLAGS) route.o $(LDLIBS) $(SHLIBS)

tftpsubs.o:	../usr.bin/tftp/tftpsubs.c
		$(CC) -c $(CFLAGS) $(DEFLIST) $?

trpt:		trpt.o
		$(CC) -o $@ $(LDFLAGS) trpt.o $(LDLIBS) $(SHLIBS)

scheme:	in.login.o security.o
		$(CC) -o $@ $(LDFLAGS) in.login.o security.o $(LDLIBS) $(SHLIBS)

#	end special targets

clean:
		rm -f *.o core a.out
		@for i in $(DIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk clean";\
			$(MAKE) -f $$i.mk clean $(MAKEARGS);\
			cd ..;\
		done;\
		wait
clobber:
		rm -f *.o arp ping scheme $(PRODUCTS) core a.out
		@for i in $(DIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk clobber";\
			$(MAKE) -f $$i.mk clobber $(MAKEARGS);\
			cd ..;\
		done;\
		wait
lintit:
		$(LINT) $(LINTFLAGS) *.c
		@for i in $(DIRS);\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk lintit";\
			$(MAKE) -f $$i.mk lintit $(MAKEARGS);\
			cd ..;\
		done;\
		wait

FRC:

#
# Header dependencies
#

arp.o:		arp.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/sys/ioctl.h \
		$(INC)/errno.h \
		$(INC)/netdb.h \
		$(INC)/nlist.h \
		$(INC)/sys/ksym.h \
		$(INC)/net/if.h \
		$(INC)/net/if_arp.h \
		$(INC)/netinet/if_ether.h \
		$(INC)/sys/sockio.h \
		$(INC)/fcntl.h \
		$(INC)/stropts.h \
		$(FRC)

gettable.o:	gettable.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		$(FRC)

ifconfig.o:	ifconfig.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/sockio.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		$(INC)/stropts.h \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/errno.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(FRC)

in.comsat.o:	in.comsat.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/wait.h \
		$(INC)/sys/file.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/sgtty.h \
		$(INC)/utmp.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/netdb.h \
		$(INC)/syslog.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(FRC)

in.fingerd.o:	in.fingerd.c \
		$(INC)/sys/types.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(FRC)

in.rarpd.o:	in.rarpd.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/sockio.h \
		$(INC)/net/if.h \
		$(INC)/net/if_arp.h \
		$(INC)/netinet/in.h \
		$(INC)/netinet/if_ether.h \
		$(INC)/sys/stropts.h \
		$(INC)/sys/dlpi.h \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/syslog.h \
		$(INC)/dirent.h \
		$(INC)/signal.h \
		$(INC)/netdb.h \
		$(FRC)

in.rexecd.o:	in.rexecd.c \
		$(INC)/sys/types.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/time.h \
		$(INC)/sys/filio.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/pwd.h \
		$(INC)/signal.h \
		$(INC)/netdb.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(FRC)

in.rlogind.o:	in.rlogind.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/wait.h \
		$(INC)/sys/file.h \
		$(INC)/sys/stream.h \
		$(INC)/sys/stropts.h \
		$(INC)/netinet/in.h \
		$(INC)/errno.h \
		$(INC)/pwd.h \
		$(INC)/signal.h \
		$(INC)/sgtty.h \
		$(INC)/fcntl.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		$(INC)/syslog.h \
		$(INC)/string.h \
		$(INC)/utmp.h \
		$(INC)/utmpx.h \
		$(INC)/sys/ttold.h \
		$(INC)/sys/filio.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(FRC)

in.rshd.o:	in.rshd.c \
		$(INC)/sys/types.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/time.h \
		$(INC)/netinet/in.h \
		$(INC)/arpa/inet.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/pwd.h \
		$(INC)/signal.h \
		$(INC)/netdb.h \
		$(INC)/syslog.h \
		$(INC)/sys/filio.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(FRC)

in.rwhod.o:	in.rwhod.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/sockio.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/file.h \
		$(INC)/sys/stropts.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		$(INC)/nlist.h \
		$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/utmp.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(INC)/syslog.h \
		$(INC)/fcntl.h \
		$(INC)/protocols/rwhod.h \
		$(FRC)

in.telnetd.o:	in.telnetd.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/wait.h \
		$(INC)/sys/file.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/filio.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stropts.h \
		$(INC)/netinet/in.h \
		$(INC)/arpa/telnet.h \
		$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/sgtty.h \
		$(INC)/netdb.h \
		$(INC)/syslog.h \
		$(INC)/ctype.h \
		$(INC)/fcntl.h \
		$(INC)/utmp.h \
		$(INC)/utmpx.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(FRC)

in.tftpd.o:	in.tftpd.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/wait.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/time.h \
		$(INC)/netinet/in.h \
		$(INC)/arpa/tftp.h \
		$(INC)/dirent.h \
		$(INC)/signal.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(INC)/setjmp.h \
		$(INC)/syslog.h \
		security.h \
		$(FRC)

in.tnamed.o:	in.tnamed.c \
		$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/signal.h \
		$(INC)/sys/time.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(FRC)

inetd.o:	inetd.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/file.h \
		$(INC)/sys/wait.h \
		$(INC)/sys/time.h \
		$(INC)/sys/resource.h \
		$(INC)/netinet/in.h \
		$(INC)/arpa/inet.h \
		$(INC)/errno.h \
		$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/netdb.h \
		$(INC)/rpc/rpcent.h \
		$(INC)/syslog.h \
		$(INC)/pwd.h \
		$(INC)/fcntl.h \
		$(INC)/tiuser.h \
		$(INC)/netdir.h \
		$(INC)/ctype.h \
		$(INC)/values.h \
		$(INC)/sys/poll.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(FRC)

ping.o:		ping.c \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/sys/types.h \
		$(INC)/sys/time.h \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/sockio.h \
		$(INC)/sys/file.h \
		$(INC)/sys/signal.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in_systm.h \
		$(INC)/netinet/in.h \
		$(INC)/netinet/ip.h \
		$(INC)/netinet/ip_icmp.h \
		$(INC)/netdb.h \
		$(FRC)

route.o:	route.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/sockio.h \
		$(INC)/sys/ioctl.h \
		$(INC)/signal.h \
		$(INC)/setjmp.h \
		$(INC)/sys/stream.h \
		$(INC)/sys/stropts.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(INC)/fcntl.h \
		$(INC)/sys/ksym.h \
		$(FRC)

trpt.o:		trpt.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/stream.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/socketvar.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in_pcb.h \
		$(INC)/netinet/in_systm.h \
		$(INC)/netinet/ip.h \
		$(INC)/netinet/ip_var.h \
		$(INC)/netinet/tcp.h \
		$(INC)/netinet/tcp_fsm.h \
		$(INC)/netinet/tcp_seq.h \
		$(INC)/netinet/tcp_timer.h \
		$(INC)/netinet/tcp_var.h \
		$(INC)/netinet/tcpip.h \
		$(INC)/netinet/tcp_debug.h \
		$(INC)/arpa/inet.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/nlist.h \
		$(INC)/sys/ksym.h \
		$(INC)/fcntl.h \
		$(INC)/sys/stat.h \
		$(FRC)

in.login.o:	in.login.c \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(FRC)
	$(CC) $(CFLAGS) $(DEFLIST) $(LICENSE) -c $<

security.o:	security.c \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(FRC)
