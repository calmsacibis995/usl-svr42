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

#	Portions Copyright (c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved.

#ident	"@(#)chgrp:chgrp.mk	1.4.7.2"
#ident "$Header: chgrp.mk 1.2 91/03/21 $"

#	Makefile for chgrp

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN=bin
GRP=bin

INCSYS = $(INC)

#top#
# Generated by makefile 1.47

MAKEFILE = chgrp.mk


MAINS = chgrp

OBJECTS =  chgrp.o

SOURCES =  chgrp.c

all:		$(MAINS)

chgrp:		chgrp.o	
	$(CC) -o chgrp  chgrp.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)


chgrp.o:	 $(INC)/stdio.h $(INC)/ctype.h \
		 $(INCSYS)/sys/types.h $(INCSYS)/sys/stat.h \
		 $(INC)/grp.h  \
		 $(INC)/dirent.h $(INC)/unistd.h \
		 $(INC)/priv.h $(INC)/locale.h \
		 $(INC)/pfmt.h $(INC)/string.h \
		 $(INC)/errno.h

GLOBALINCS = $(INC)/ctype.h $(INC)/grp.h $(INC)/stdio.h \
	$(INCSYS)/sys/stat.h $(INCSYS)/sys/types.h \
	$(INC)/dirent.h $(INC)/unistd.h $(INC)/priv.h $(INC)/locale.h \
	$(INC)/pfmt.h $(INC)/string.h $(INC)/errno.h


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(INSDIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(INSDIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)