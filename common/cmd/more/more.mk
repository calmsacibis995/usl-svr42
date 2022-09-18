#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)more:more.mk	1.7.6.3"
#ident "$Header: more.mk 1.2 91/04/17 $"

include $(CMDRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved
#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.

OWN = bin
GRP = bin

LDLIBS = -lcmd -lcurses -lgen

DATA = more.help

all: more

more: more.o 
	$(CC) -o more more.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

more.o: more.c \
	$(INC)/ctype.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/curses.h \
	$(INC)/term.h \
	$(INC)/sys/ioctl.h \
	$(INC)/setjmp.h \
	$(INC)/sys/stat.h \
	$(INC)/values.h \
	$(INC)/stdlib.h

install: more $(USRBIN)
	$(INS) -f $(USRBIN) -m 555 -u $(OWN) -g $(GRP) more
	rm -f $(USRBIN)/page
	ln $(USRBIN)/more $(USRBIN)/page
	$(INS) -f $(USRLIB) -m 0644 -u $(OWN) -g $(GRP) $(DATA)

clean:
	rm -f more.o
	
clobber: clean
	rm -f more

lintit:
	$(LINT) $(LINTFLAGS) more.c

#	optional targets

save:
	cd $(USRBIN); set -x; for m in more; do cp $$m OLD$$m; done
	cd $(USRLIB); set -x; for m in $(DATA); do cp $$m OLD$$m; done

restore:
	cd $(USRBIN); set -x; for m in more; do; cp OLD$$m $$m; done
	cd $(USRLIB); set -x; for m in $(DATA); do; cp OLD$$m $$m; done

remove:
	cd $(USRBIN); rm -f more
	cd $(USRLIB); rm -f $(DATA)

partslist:
	@echo more.mk $(LOCALINCS) more.c | tr ' ' '\012' | sort

product:
	@echo more | tr ' ' '\012' | \
	sed -e 's;^;$(USRBIN)/;' -e 's;//*;/;g'

productdir:
	@echo $(USRBIN)
