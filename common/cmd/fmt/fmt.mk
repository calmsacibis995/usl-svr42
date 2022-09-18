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
# 	All Rights Reserved.

#ident	"@(#)fmt:fmt.mk	1.3.3.1"
#ident "$Header: fmt.mk 1.2 91/04/09 $"

#     Makefile for fmt 

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LOCHDR = hdr
LOCALINC = -I$(LOCHDR)

#top#

MAKEFILE = fmt.mk

MAINS = fmt 

OBJECTS =  fmt.o head.o

SOURCES =  fmt.c head.c

all:          $(MAINS)

$(MAINS):	fmt.o head.o
	$(CC) -o $@ fmt.o head.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	
fmt.o:		$(INC)/stdio.h \
		$(INC)/ctype.h

head.o:		$(LOCHDR)/def.h $(LOCHDR)/glob.h \
		$(LOCHDR)/local.h $(INC)/stdio.h \
		$(LOCHDR)/v7.local.h $(LOCHDR)/usg.local.h \
		$(INC)/sys/param.h $(INC)/signal.h \
		$(INC)/stdio.h 

GLOBALINCS = $(INC)/stdio.h $(INC)/ctype.h \
		$(LOCHDR)/local.h $(INC)/stdio.h \
		$(LOCHDR)/v7.local.h $(LOCHDR)/usg.local.h \
		$(INC)/sys/param.h $(INC)/signal.h 

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

newmakefile:
	makefile -m -f $(MAKEFILE)  -s INC $(INC)
#bottom#

install:	all
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) fmt

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

#     These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS)  |  tr ' ' '\012'  |  sort

productdir:
	@echo $(INSDIR) | tr ' ' '\012' | sort

product:
	@echo $(MAINS)  |  tr ' ' '\012'  | \
	sed 's;^;$(INSDIR)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)

