#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)passwd:passwd.mk	1.5.7.6"
#ident  "$Header: passwd.mk 1.3 91/06/24 $"

include $(CMDRULES)

#	Makefile for passwd

OWN = root
GRP = sys

LDLIBS = -lcmd -lcrypt_i -lia -lgen -liaf

all: passwd

passwd: passwd.o 
	$(CC) -o passwd passwd.o  $(LDFLAGS) $(LDLIBS) 

passwd.o: passwd.c \
	$(INC)/stdio.h \
	$(INC)/signal.h \
	$(INC)/pwd.h \
	$(INC)/shadow.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/vnode.h \
	$(INC)/time.h \
	$(INC)/string.h \
	$(INC)/ctype.h \
	$(INC)/errno.h \
	$(INC)/crypt.h \
	$(INC)/deflt.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/sys/mman.h \
	$(INC)/fcntl.h \
	$(INC)/ia.h \
	$(INC)/audit.h \
	$(INC)/priv.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/systeminfo.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

install: all
	$(INS) -f $(USRBIN) -m 06555 -u $(OWN) -g $(GRP) passwd
	-mkdir ./tmp
	-$(CP) passwd.dfl ./tmp/passwd
	$(INS) -f $(ETC)/default -m 0444 -u $(OWN) -g $(GRP) ./tmp/passwd
	-rm -rf ./tmp

clean:
	rm -f passwd.o

clobber: clean
	rm -f passwd

lintit:
	$(LINT) $(LINTFLAGS) passwd.c

#	These targets are useful but optional

partslist:
	@echo passwd.mk passwd.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo passwd | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit passwd.mk $(LOCALINCS) passwd.c -o passwd.o passwd
