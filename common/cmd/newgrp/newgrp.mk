#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)newgrp:newgrp.mk	1.6.10.2"
#ident  "$Header: newgrp.mk 1.3 91/06/28 $"

include $(CMDRULES)

#	Makefile for <newgrp>

OWN = root
GRP = sys

LDLIBS = -lcrypt_i

all: newgrp

newgrp: newgrp.o 
	$(CC) -o newgrp newgrp.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

newgrp.o: newgrp.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/pwd.h \
	$(INC)/grp.h \
	$(INC)/crypt.h \
	$(INC)/string.h \
	$(INC)/stdlib.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sys/secsys.h \
	$(INC)/priv.h

install: all
	 $(INS) -f $(USRBIN) -m 04755 -u $(OWN) -g $(GRP) newgrp

clean:
	rm -f newgrp.o

clobber: clean
	rm -f newgrp

lintit:
	$(LINT) $(LINTFLAGS) newgrp.c

#	These targets are useful but optional

partslist:
	@echo newgrp.mk newgrp.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo newgrp | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit newgrp.mk $(LOCALINCS) newgrp.c -o newgrp.o newgrp
