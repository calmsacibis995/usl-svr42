#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)nsquery:nsquery.mk	1.5.10.2"
#ident "$Header: nsquery.mk 1.2 91/04/16 $"

include $(CMDRULES)


OWN = root
GRP = bin

LDLIBS = -lns

all: nsquery

nsquery: nsquery.o 
	$(CC) -o nsquery nsquery.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

nsquery.o: nsquery.c \
	$(INC)/sys/types.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/list.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/nserve.h \
	$(INC)/errno.h $(INC)/sys/errno.h

install: all
	-rm -f $(USRBIN)/nsquery
	 $(INS) -f $(USRSBIN) -m 04555 -u $(OWN) -g $(GRP) nsquery
	-$(SYMLINK) /usr/sbin/nsquery $(USRBIN)/nsquery

clean:
	rm -f nsquery.o

clobber: clean
	rm -f nsquery

lintit:
	$(LINT) $(LINTFLAGS) nsquery.c
