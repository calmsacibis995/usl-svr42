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

#ident	"@(#)grep:common/cmd/grep/grep.mk	1.6.7.2"
#ident "$Header: grep.mk 1.2 91/04/24 $"

#	Makefile for grep 

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LDLIBS= -lgen

MAKEFILE = grep.mk


MAINS = grep grep.dy

OBJECTS =  grep.o

SOURCES =  grep.c

all:		$(MAINS)

grep:		grep.o 
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

grep.dy:		grep.o 
	$(CC) -o $@ grep.o $(LDFLAGS) $(LDLIBS) -dy


grep.o:		 $(INC)/ctype.h $(INC)/memory.h \
		 $(INC)/regexpr.h $(INC)/stdio.h \
		 $(INC)/sys/types.h $(INC)/locale.h \
		 $(INC)/pfmt.h $(INC)/string.h \
		 $(INC)/errno.h $(INC)/unistd.h

GLOBALINCS = $(INC)/ctype.h $(INC)/memory.h \
	$(INC)/regexpr.h $(INC)/stdio.h \
	$(INC)/sys/types.h $(INC)/locale.h \
	$(INC)/pfmt.h $(INC)/string.h \
	$(INC)/errno.h $(INC)/unistd.h

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install: all
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) grep
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) grep
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) grep.dy
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) grep.dy

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(DIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(DIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
