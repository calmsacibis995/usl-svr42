#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfstop:rfstop.mk	1.7.7.2"
#ident "$Header: rfstop.mk 1.2 91/04/10 $"

include $(CMDRULES)


OWN = bin
GRP = bin

LDLIBS = -lns

all: rfstop

rfstop: rfstop.o 
	$(CC) -o rfstop rfstop.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rfstop.o: rfstop.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/nserve.h \
	$(INC)/time.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/signal.h

install: all
	-rm -f $(USRBIN)/rfstop
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) rfstop
	-$(SYMLINK) /usr/sbin/rfstop $(USRBIN)/rfstop

clean:
	rm -f rfstop.o

clobber: clean
	rm -f rfstop

lintit:
	$(LINT) $(LINTFLAGS) rfstop.c
