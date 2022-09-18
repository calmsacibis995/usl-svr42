#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfadmin:rfadmin.mk	1.2.13.2"
#ident  "$Header: rfadmin.mk 1.3 91/06/28 $"

include $(CMDRULES)


OWN = bin
GRP = bin

LDLIBS = -lns -lcrypt_i 

all: rfadmin

rfadmin: rfadmin.o 
	$(CC) -o rfadmin rfadmin.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rfadmin.o: rfadmin.c \
	$(INC)/sys/types.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/list.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/nserve.h \
	$(INC)/nsaddr.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/rf_comm.h \
	$(INC)/sys/rf_debug.h \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/nserve.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/time.h

install: all
	-rm -f $(USRBIN)/rfadmin
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) rfadmin
	-$(SYMLINK) /usr/sbin/rfadmin $(USRBIN)/rfadmin

clean:
	rm -f rfadmin.o

clobber: clean
	rm -f rfadmin

lintit:
	$(LINT) $(LINTFLAGS) rfadmin.c
