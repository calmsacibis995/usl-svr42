#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rexec:rexec.mk	1.7.3.3"
#ident  "$Header: rexec.mk 1.8 91/07/05 $"

include $(CMDRULES)

#	Makefile for Remote Execution

OWN = bin
GRP = bin

INSADMIN = $(USRSBIN)
INSUSER  = $(USRBIN)
LINSUSER = /usr/bin
INSETC   = $(ETC)/rexec

LOCALDEFS = -DSYSV
MAINS = rexec rxserver rxlist rxservice

OBJECTS = rexec.o \
	rxserver.o rxs_net.o rxs_ptty.o rxs_pipe.o rxs_child.o rxputm.o \
	rxlist.o rxservice.o \
	log.o svcent.o rxperror.o

SOURCES = $(OBJECTS:.o=.c)

all: $(MAINS)

rexec: rexec.o rxperror.o
	$(CC) -o rexec rexec.o rxperror.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) -lnsl

rxserver: rxserver.o rxs_net.o rxs_ptty.o rxs_pipe.o rxs_child.o rxputm.o \
	log.o svcent.o
	$(CC) -o rxserver \
	rxserver.o rxs_net.o rxs_ptty.o rxs_pipe.o rxs_child.o rxputm.o \
	log.o svcent.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) -lnsl -liaf

rxlist: rxlist.o svcent.o
	$(CC) -o rxlist rxlist.o svcent.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rxservice: rxservice.o svcent.o
	$(CC) -o rxservice rxservice.o svcent.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rexec.o: $(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/string.h \
	$(INC)/stropts.h \
	$(INC)/errno.h \
	$(INC)/poll.h \
	$(INC)/sys/termios.h \
	$(INC)/signal.h \
	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/rx.h

rxserver.o: $(INC)/poll.h \
	$(INC)/errno.h \
	$(INC)/rx.h \
	rxmsg.h

rxs_net.o: $(INC)/sys/byteorder.h \
	$(INC)/errno.h \
	$(INC)/unistd.h \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/rx.h \
	rxmsg.h \
	rxsvcent.h

rxs_ptty.o: $(INC)/sys/byteorder.h \
	$(INC)/termio.h \
	$(INC)/stropts.h \
	$(INC)/sys/stream.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h \
	$(INC)/stdio.h \
	$(INC)/rx.h \
	rxmsg.h

rxs_pipe.o: $(INC)/sys/byteorder.h \
	$(INC)/rx.h \
	rxmsg.h

rxs_child.o: $(INC)/sys/byteorder.h \
	$(INC)/fcntl.h \
	$(INC)/priv.h \
	$(INC)/wait.h \
	$(INC)/rx.h \
	rxmsg.h

rxputm.o: $(INC)/sys/byteorder.h \
	$(INC)/rx.h \
	rxmsg.h

rxlist.o: $(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/unistd.h \
	$(INC)/pfmt.h \
	$(INC)/rx.h \
	rxsvcent.h

rxservice.o: $(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/unistd.h \
	$(INC)/pfmt.h \
	$(INC)/rx.h \
	rxsvcent.h

log.o: $(INC)/stdio.h

svcent.o: $(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/rx.h \
	rxsvcent.h

rxperror.o: $(INC)/pfmt.h \
	$(INC)/rx.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

install: all
	$(INS) -f $(INSUSER) -m 0555 -u $(OWN) -g $(GRP) rexec
	rm -f $(INSUSER)/rx
	$(SYMLINK) $(LINSUSER)/rexec $(INSUSER)/rx
	rm -f $(INSUSER)/rl
	$(SYMLINK) $(LINSUSER)/rexec $(INSUSER)/rl
	rm -f $(INSUSER)/rquery
	$(SYMLINK) $(LINSUSER)/rexec $(INSUSER)/rquery
	[ -d $(USRLIB)/rexec ] || mkdir -p $(USRLIB)/rexec ;\
		$(CH)chown $(OWN) $(USRLIB)/rexec ;\
		$(CH)chgrp $(GRP) $(USRLIB)/rexec ;\
		$(CH)chmod 0755 $(USRLIB)/rexec
	[ -d $(INSETC) ] || mkdir -p $(INSETC) ;\
		$(CH)chown $(OWN) $(INSETC) ;\
		$(CH)chgrp $(GRP) $(INSETC) ;\
		$(CH)chmod 0755 $(INSETC)
	$(INS) -f $(USRLIB)/rexec -m 0500 -u $(OWN) -g $(GRP) rxserver
	$(INS) -f $(USRLIB)/rexec -m 0555 -u $(OWN) -g $(GRP) rxlist
	$(INS) -f $(INSADMIN) -m 0555 -u $(OWN) -g $(GRP) rxservice
	$(INS) -f $(INSETC) -m 0644 -u $(OWN) -g $(GRP) services

#	These targets are useful but optional

partslist:
	@echo rexec.mk $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo  | tr ' ' '\012' | sort

product:
	@echo $(MAINS) | tr ' ' '\012' | \
	sed 's;^;/;'

srcaudit:
	@fileaudit rexec.mk $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
