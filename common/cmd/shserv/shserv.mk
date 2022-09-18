#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)shserv:shserv.mk	1.6.1.2"
#ident "$Header: shserv.mk 1.2 91/03/19 $"

include $(CMDRULES)

OWN = root
GRP = bin

LDLIBS = -lcmd -lgen -liaf 

all: shserv

shserv: shserv.o 
	$(CC) -o shserv shserv.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

shserv.o: shserv.c \
	$(INC)/stdio.h \
	$(INC)/iaf.h \
	$(INC)/unistd.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/string.h \
	$(INC)/sys/secsys.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h

install: shserv 
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) shserv

clean:
	rm -f shserv.o
	
clobber: clean
	rm -f shserv

lintit:
	$(LINT) $(LINTFLAGS) shserv.c

# optional targets

save:
	cd $(USRBIN); set -x; for m in shserv; do cp $$m OLD$$m; done

restore:
	cd $(USRBIN); set -x; for m in shserv; do; cp OLD$$m $$m; done

remove:
	cd $(USRBIN); rm -f shserv

partslist:
	@echo shserv.mk $(LOCALINCS) shserv.c | tr ' ' '\012' | sort

product:
	@echo shserv | tr ' ' '\012' | \
	sed -e 's;^;$(USRBIN)/;' -e 's;//*;/;g'

productdir:
	@echo $(USRBIN)

srcaudit: # will not report missing nor present object or product files.
	@fileaudit shserv.mk $(LOCALINCS) shserv.c -o shserv.o shserv
