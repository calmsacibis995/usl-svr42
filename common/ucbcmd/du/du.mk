#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/du/du.mk	1.2"
#ident	"$Header: $"
# 	Protions Copyright(c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved


#	Makefile for du 

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = du.mk

MAINS = du

OBJECTS =  du.o

SOURCES =  du.c

ALL:		$(MAINS)

du:		du.o 
	$(CC) -o du  du.o   $(LDFLAGS) $(PERFLIBS)


du.o:		 $(INC)/stdio.h $(INC)/sys/types.h \
		 $(INC)/sys/param.h $(INC)/sys/stat.h \
		 $(INC)/sys/dirent.h 

GLOBALINCS = $(INC)/stdio.h \
	$(INC)/sys/param.h $(INC)/sys/stat.h \
	$(INC)/sys/types.h  $(INC)/sys/dirent.h


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 0555 $(MAINS)

