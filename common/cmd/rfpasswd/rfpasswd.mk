#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfpasswd:rfpasswd.mk	1.9.10.2"
#ident  "$Header: rfpasswd.mk 1.3 91/06/28 $"

include $(CMDRULES)


OWN = bin
GRP = bin

LDLIBS = -lns -lcrypt_i 

all: rfpasswd

rfpasswd: rfpasswd.o 
	$(CC) -o rfpasswd rfpasswd.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

rfpasswd.o: rfpasswd.c \
	$(INC)/stdio.h \
	$(INC)/ctype.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/utsname.h \
	$(INC)/nserve.h \
	$(INC)/time.h \
	$(INC)/errno.h $(INC)/sys/errno.h

install: all
	-rm -f $(USRBIN)/rfpasswd
	 $(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) rfpasswd
	-$(SYMLINK) /usr/sbin/rfpasswd $(USRBIN)/rfpasswd

clean:
	rm -f rfpasswd.o

clobber: clean
	rm -f rfpasswd

lintit:
	$(LINT) $(LINTFLAGS) rfpasswd.c
