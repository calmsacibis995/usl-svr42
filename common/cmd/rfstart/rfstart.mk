#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfstart:rfstart.mk	1.7.7.2"
#ident "$Header: rfstart.mk 1.2 91/04/10 $"

include $(CMDRULES)


OWN = bin
GRP = bin

LDLIBS = -lns

all: rfstart

rfstart: rfstart.o 
	$(CC) -o rfstart rfstart.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rfstart.o: rfstart.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/fcntl.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/sys/utsname.h \
	$(INC)/nserve.h \
	$(INC)/time.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/ctype.h \
	$(INC)/mac.h

install: all
	-rm -f $(USRBIN)/rfstart
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) rfstart
	-$(SYMLINK) /usr/sbin/rfstart $(USRBIN)/rfstart

clean:
	rm -f rfstart.o

clobber: clean
	rm -f rfstart

lintit:
	$(LINT) $(LINTFLAGS) rfstart.c
