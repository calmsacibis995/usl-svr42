#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)od:od.mk	1.4.5.1"
#ident "$Header: od.mk 1.2 91/04/16 $"

include $(CMDRULES)


OWN = bin
GRP = bin

all: od

od: od.o 
	$(CC) -o od od.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

od.o: od.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/ctype.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h

install: od
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) od

clean:
	rm -f od.o
	
clobber: clean
	rm -f od

lintit:
	$(LINT) $(LINTFLAGS) od.c

#	These are optional but useful targets

partslist:
	@echo od.mk $(LOCALINCS) od.c | tr ' ' '\012' | sort

product:
	@echo od | tr ' ' '\012' | \
	sed -e 's;^;$(USRBIN)/;' -e 's;//*;/;g'

productdir:
	@echo $(USRBIN)
