
LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP
LDLIBS=		-L/usr/lib -lsocket -lnsl
CC=cc -O
.c.o:
		$(CC) $(LOCALDEF)  $(LDLIBS) -c $<

all:		rlogin

rlogin:		rlogin.o security.o
		$(CC) -o $@ $(LOCALDEF) $(LDLIBS) rlogin.o security.o 

rlogin.o:	rlogin.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/file.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/wait.h \
		$(INC)/sys/stropts.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/sgtty.h \
		$(INC)/pwd.h \
		$(INC)/signal.h \
		$(INC)/setjmp.h \
		$(INC)/netdb.h \
		$(INC)/fcntl.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/sockio.h \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \

security.o:	../usr.sbin/security.c \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../usr.sbin/security.h \
		$(CC) $(LOCALDEF) $(LDLIBS) -I../usr.sbin -c ../usr.sbin/security.c \

