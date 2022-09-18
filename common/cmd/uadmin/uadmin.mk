#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)uadmin:uadmin.mk	1.10.3.2"
#ident "$Header: uadmin.mk 1.2 91/03/14 $"

include $(CMDRULES)

OWN = root
GRP = sys

all: uadmin

uadmin: uadmin.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/sys/signal.h \
	$(INC)/priv.h \
	$(INC)/sys/uadmin.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@.dy $@.c $(LDFLAGS) $(LDLIBS)

install: all
	-rm -f $(ETC)/uadmin
	-rm -f $(ETC)/uadmin.dy
	-rm -f $(USRSBIN)/uadmin
	-rm -f $(USRSBIN)/uadmin.dy
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) uadmin
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) uadmin
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) uadmin.dy
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) uadmin.dy
	-$(SYMLINK) /sbin/uadmin $(ETC)/uadmin
	-$(SYMLINK) /sbin/uadmin.dy $(ETC)/uadmin.dy

clean:
	rm -f *.o

clobber: clean
	rm -f uadmin uadmin.dy

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
