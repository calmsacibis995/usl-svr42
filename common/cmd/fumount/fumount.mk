#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)fumount:fumount.mk	1.4.16.2"
#ident "$Header: fumount.mk 1.2 91/04/24 $"

#	fumount make file

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin

all:	fumount

fumount: fumount.o sndmes.o
	$(CC) -o $@ $(LDFLAGS) fumount.o sndmes.o $(LDLIBS) $(SHLIBS)
	$(CH)chmod 755 fumount

fumount.o: fumount.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/idtab.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/nserve.h 

sndmes.o: sndmes.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/rf_messg.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_sys.h 

install: fumount
	-rm -f $(USRBIN)/fumount
	$(INS) -f $(INSDIR) -m 555 -u $(OWN) -g $(GRP) fumount
	-$(SYMLINK) /usr/sbin/fumount $(USRBIN)/fumount

clean:
	-rm -f fumount.o sndmes.o

clobber: clean
	rm -f fumount

