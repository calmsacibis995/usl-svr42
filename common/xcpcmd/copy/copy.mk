#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcpcopy:copy.mk	1.5.3.2"
#ident  "$Header: copy.mk 1.2 91/07/11 $"

include $(CMDRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved
#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.

OWN = bin
GRP = bin

#	Where MAINS are to be installed.
INSDIR = $(USRBIN)

all: copy

copy: copy.o 
	$(CC) -o copy copy.o  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

copy.o: copy.c \
	$(INC)/stdio.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/fcntl.h \
	$(INC)/utime.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/dirent.h $(INC)/sys/dirent.h

install: copy
	 $(INS) -f $(INSDIR) -m 0711 -u $(OWN) -g $(GRP) copy 

clean:
	rm -f copy.o
	
clobber: clean
	rm -f copy

lintit:
	$(LINT) $(LINTFLAGS) copy.c

# These are optional but useful targets

save:
	cd $(INSDIR); set -x; for m in copy; do cp $$m OLD$$m; done

restore:
	cd $(INSDIR); set -x; for m in copy; do; cp OLD$$m $$m; done

remove:
	cd $(INSDIR); rm -f copy

partslist:
	@echo copy.mk $(LOCALINCS) copy.c | tr ' ' '\012' | sort

product:
	@echo copy | tr ' ' '\012' | \
	sed -e 's;^;$(INSDIR)/;' -e 's;//*;/;g'

productdir:
	@echo $(INSDIR)

srcaudit: # will not report missing nor present object or product files.
	@fileaudit copy.mk $(LOCALINCS) copy.c -o copy.o copy
