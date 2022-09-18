#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)unadv:unadv.mk	1.5.8.2"
#ident "$Header: unadv.mk 1.3 91/03/19 $"

include $(CMDRULES)


OWN = bin
GRP = bin

LDLIBS = -lns

all: unadv

unadv: unadv.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/nserve.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/rf_sys.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	-rm -f $(USRBIN)/unadv
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) unadv
	-$(SYMLINK) /usr/sbin/unadv $(USRBIN)/unadv

clean:
	rm -f unadv.o

clobber: clean
	rm -f unadv

lintit:
	$(LINT) $(LINTFLAGS) unadv.c
