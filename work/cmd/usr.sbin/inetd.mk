INC=/usr/include
CC=cc -O
LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP

LDLIBS=		-L/usr/lib -lsocket -lnsl -lcrypt_i -lgen -lia -liaf 

.c.o:
		$(CC) $(LOCALDEF) $(LDLIBS) -c $< 

all:		inetd	in.rlogind	

#	these targets use default rule
in.rlogind:	in.rlogind.o security.o
		cc -o $@ $(LOCALDEF) $(LDLIBS) in.rlogind.o security.o 
inetd:		inetd.o security.o
		cc -o $@ $(LOCALDEF) $(LDLIBS) inetd.o security.o 


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

security.o:	security.c \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
