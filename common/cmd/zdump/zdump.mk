#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)zdump:zdump.mk	1.1.5.3"
#ident "$Header: $"

include $(CMDRULES)

#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
# Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
# (c) 1986,1987,1988,1989 Sun Microsystems, Inc
# (c) 1983,1984,1985,1986,1987,1988,1989 AT&T.
#      All rights reserved.

OWN = bin
GRP = bin

OBJECTS = zdump.o ialloc.o #time_comm.o

SOURCES = zdump.c ialloc.o #time_comm.o

all: zdump

install: all
	$(INS) -f $(USRSBIN) -u $(OWN) -g $(GRP) -m 0555 zdump

zdump: $(OBJECTS)
	$(CC) -o zdump $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(SHLIBS)

ialloc.o: $(INC)/sys/types.h \
	time.h \
	$(INC)/stdio.h \
	$(INC)/tzfile.h 

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f zdump

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

# These targets are useful but optional

partslist:
	@echo zdump.mk $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRSBIN) | tr ' ' '\012' | sort

product:
	@echo zdump | tr ' ' '\012' | \
	sed 's;^;$(USRSBIN)/;'

srcaudit:
	@fileaudit zdump.mk $(LOCALINCS) $(SOURCES) -o $(OBJECTS) zdump
