#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)creatiadb:creatiadb.mk	1.1.5.2"
#ident "$Header: creatiadb.mk 1.4 91/04/11 $"

include $(CMDRULES)

OWN = root
GRP = sys

#	Common Libraries and -l<lib> flags.
LDLIBS = -lia -lgen 

all: creatiadb

creatiadb: creatiadb.o 
	$(CC) -o creatiadb creatiadb.o $(LDFLAGS) $(LDLIBS) $(SHLIBS) $(ROOTLIBS)

creatiadb.o: creatiadb.c \
	$(INC)/stdio.h \
	$(INC)/pwd.h \
	$(INC)/shadow.h \
	$(INC)/grp.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/time.h \
	$(INC)/sys/time.h \
	$(INC)/sys/mac.h \
	$(INC)/audit.h \
	$(INC)/ia.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/stdlib.h \
	$(INC)/unistd.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

clean:
	rm -f creatiadb.o
	
clobber: clean
	rm -f creatiadb

lintit:
	$(LINT) $(LINTFLAGS) creatiadb.c

install: creatiadb
	 $(INS) -f $(SBIN) -m 0500 -u $(OWN) -g $(GRP) creatiadb

remove:
	cd $(SBIN); rm -f creatiadb


partslist:
	@echo creatiadb.mk $(LOCALINCS) creatiadb.c | tr ' ' '\012' | sort

product:
	@echo creatiadb | tr ' ' '\012' | \
	sed -e 's;^;$(SBIN)/;' -e 's;//*;/;g'

productdir:
	@echo $(SBIN)
