#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcprandom:random.mk	1.1.5.2"
#ident  "$Header: random.mk 1.2 91/07/11 $"

include $(CMDRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved
#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.
#	Makefile for random

OWN = root
GRP = bin

#	Where MAINS are to be installed.
INSDIR = $(USRBIN)

all: random

random: random.o 
	$(CC) -o random random.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

random.o: random.c \
	$(INC)/stdio.h

install: random
	 $(INS) -f $(INSDIR) -m 0711 -u $(OWN) -g $(GRP) random

clean:
	rm -f random.o
	
clobber: clean
	rm -f random

lintit:
	$(LINT) $(LINTFLAGS) random.c

# These are optional but useful targets

save:
	cd $(INSDIR); set -x; for m in random; do cp $$m OLD$$m; done

restore:
	cd $(INSDIR); set -x; for m in random; do; cp OLD$$m $$m; done

remove:
	cd $(INSDIR); rm -f random

partslist:
	@echo random.mk $(LOCALINCS) random.c | tr ' ' '\012' | sort

product:
	@echo random | tr ' ' '\012' | \
	sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)

srcaudit: # will not report missing nor present object or product files.
	@fileaudit random.mk $(LOCALINCS) random.c -o random.o random
