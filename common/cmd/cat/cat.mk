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

#ident	"@(#)cat:cat.mk	1.4.6.1"
#ident "$Header: cat.mk 1.3 91/03/21 $"

include $(CMDRULES)

#	Makefile for cat

OWN = bin
GRP = bin

SYMLINK = :

LDLIBS = -lw

#top#
# Generated by makefile 1.47

MAKEFILE = cat.mk


MAINS = cat

OBJECTS =  cat.o

SOURCES =  cat.c

all:		$(MAINS)

cat:		cat.o 
	$(CC) -o cat  cat.o   $(LDFLAGS) $(LDLIBS) $(PERFLIBS)


cat.o:		 $(INC)/stdio.h $(INC)/ctype.h \
		 $(INC)/sys/types.h \
		 $(INC)/sys/stat.h  \
		 $(INC)/sys/euc.h \
		 $(INC)/getwidth.h \
		 $(INC)/locale.h \
		 $(INC)/pfmt.h \
		 $(INC)/errno.h \
		 $(INC)/string.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	 $(INS) -f  $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

lintit:

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
