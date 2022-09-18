#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)fusage:fusage.mk	1.3.14.4"
#ident "$Header: fusage.mk 1.3 91/05/22 $"

include $(CMDRULES)


OWN = bin
GRP = sys

all: fusage

fusage: fusage.o 
	$(CC) -o fusage fusage.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

fusage.o: fusage.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/statfs.h \
	$(INC)/sys/mnttab.h \
	$(INC)/sys/utsname.h \
	$(INC)/nserve.h \
	$(INC)/ctype.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/nserve.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/sys/ksym.h \
	$(INC)/sys/vfs.h \
	$(INC)/errno.h $(INC)/sys/errno.h

install: all
	-rm -f $(USRBIN)/fusage
	 $(INS) -f $(USRSBIN) -m 02555 -u $(OWN) -g $(GRP) fusage
	-$(SYMLINK) /usr/sbin/fusage $(USRBIN)/fusage

clean:
	-rm -f fusage.o

clobber: clean
	rm -f fusage

lintit:
	$(LINT) $(LINTFLAGS) fusage.c
