#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)idload:idload.mk	1.4.14.4"
#ident "$Header: idload.mk 1.3 91/05/22 $"

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin

LDLIBS= -lns
FRC =

all: idload

idload: idload.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

idload.o: idload.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/idtab.h \
		$(INC)/sys/nserve.h \
		$(INC)/sys/rf_sys.h \
		$(INC)/fcntl.h \
		$(INC)/sys/ksym.h\
		$(INC)/nserve.h \
		$(INC)/sys/param.h \
		$(INC)/sys/tiuser.h \
		$(INC)/sys/stat.h \
		$(INC)/nsaddr.h \
		$(INC)/sys/rf_cirmgr.h \
		$(INC)/pn.h

install: all
	-rm -f $(USRBIN)/idload
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) idload
	-$(SYMLINK) /usr/sbin/idload $(USRBIN)/idload

clean:
	rm -f *.o

clobber: clean
	rm -f $(TESTDIR)/idload
FRC:
