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

#ident	"@(#)id:id.mk	1.4.5.1"
#ident "$Header: id.mk 1.2 91/04/25 $"

#	Makefile for id

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

#top#

MAKEFILE = id.mk

MAINS = id

OBJECTS =  id.o

SOURCES =  id.c

all:		$(MAINS)

id:	id.o
	$(CC) -o id id.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

id.o:		 $(INC)/stdio.h \
		 $(INC)/pwd.h \
		 $(INC)/grp.h \
		 $(INC)/sys/param.h

GLOBALINCS = $(INC)/grp.h \
	$(INC)/pwd.h \
	$(INC)/stdio.h \
	$(INC)/sys/param.h


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) id

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
