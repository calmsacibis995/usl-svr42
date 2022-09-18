#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfudaemon:rfudaemon.mk	1.1.12.2"
#ident "$Header: rfudaemon.mk 1.2 91/04/10 $"

include $(CMDRULES)


OWN = bin
GRP = bin

INSDIR = $(USRLIB)/rfs

all: rfudaemon

rfudaemon: rfudaemon.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rfudaemon.o: rfudaemon.c \
	$(INC)/sys/types.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/sys/signal.h \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/resource.h

install: all
	-@if [ ! -d "$(INSDIR)" ] ; \
	then \
	mkdir $(INSDIR) ; \
	fi ;
	-rm -f $(ROOT)/usr/nserve/rfudaemon
	 $(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) rfudaemon
	-@if [ ! -d "$(USRNSERVE)" ] ; \
	then \
	mkdir $(USRNSERVE) ; \
	fi ;
	-$(SYMLINK) /usr/lib/rfs/rfudaemon $(USRNSERVE)/rfudaemon

clean:
	rm -f *.o

clobber: clean
	rm -f rfudaemon

lintit:
	$(LINT) $(LINTFLAGS) *.c
