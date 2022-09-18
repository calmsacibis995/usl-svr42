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

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.routed/tools/tools.mk	1.1.2.1"
#ident "$Header: tools.mk 1.3 91/06/26 $"

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl

all:		query trace

query:		query.o
		$(CC) -o query $(LDFLAGS) query.o $(LDLIBS) $(SHLIBS)

trace:		trace.o
		$(CC) -o trace $(LDFLAGS) trace.o $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) query
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) trace

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f query trace

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:


query.o:	query.c \
		$(INC)/sys/param.h \
		$(INC)/sys/protosw.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/time.h \
		$(INC)/netinet/in.h \
		$(INC)/errno.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		$(INC)/protocols/routed.h \
		$(INC)/signal.h \
		$(FRC)

trace.o:	trace.c \
		$(INC)/sys/param.h \
		$(INC)/sys/protosw.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/errno.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		$(INC)/protocols/routed.h \
		$(FRC)
