#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cmd-streams:kmacct/kmacct.mk	1.6.4.2"
#ident "$Header: kmacct.mk 1.3 91/04/29 $"

include $(CMDRULES)

OWN = root
GRP = sys

MAINS = kmacntl kmapr kmamkdb
SOURCES = kmacntl.c kmapr.c

all: $(MAINS)

kmacntl: kmacntl.c \
	$(INC)/sys/types.h \
	$(INC)/sys/kmacct.h \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

kmapr: kmapr.c \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/kmacct.h \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/fcntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	for n in $(MAINS) ; do \
	 	$(INS) -f $(USRSBIN) -m 0100 -u $(OWN) -g $(GRP) $$n ; \
	done
	
clean:
	rm -f *.o

clobber: clean
	-rm -f kmacntl kmapr

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
