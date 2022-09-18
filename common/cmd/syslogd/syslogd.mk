#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)syslogd:syslogd.mk	1.4.3.1"
#ident "$Header: syslogd.mk 1.3 91/03/19 $"

include $(CMDRULES)

#
# syslogd.mk:
# makefile for syslogd(1M) daemon
#

OWN = root
GRP = sys

ELFLIBS = -lnsl
COFFLIBS = -lnsl_s -lc_s

all: syslogd

syslogd: syslogd.o
	if [ x$(CCSTYPE) = xCOFF ] ; \
	then \
	 	$(CC) -o syslogd syslogd.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(COFFLIBS) ; \
	else \
	 	$(CC) -o syslogd syslogd.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(ELFLIBS) ; \
	fi

syslogd.o: syslogd.c \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/string.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h \
	$(INC)/tiuser.h \
	$(INC)/utmp.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stropts.h \
	$(INC)/sys/syslog.h \
	$(INC)/sys/strlog.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/time.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/poll.h \
	$(INC)/sys/wait.h

install: all
	 $(INS) -f $(USRSBIN) -m 0100 -u $(OWN) -g $(GRP) syslogd

clean:
	-rm -f syslogd.o

clobber: clean
	-rm -f syslogd

lintit:
	$(LINT) $(LINTFLAGS) syslogd

