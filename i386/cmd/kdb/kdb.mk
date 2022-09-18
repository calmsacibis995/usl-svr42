#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kdb.cmd:i386/cmd/kdb/kdb.mk	1.3"

include	$(CMDRULES)


all:	kdb

kdb: \
	kdb.o \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/sysi86.h \
	$(INC)/errno.h
	$(CC) $(LDFLAGS) -o $@ $@.o $(ROOTLIBS)

install: all $(SBIN)
	$(INS) -f $(SBIN) kdb

$(SBIN):
	-mkdir -p $@

clean:
	rm -f *.o 

clobber:	clean
	rm -f kdb
