#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/pwck/pwck.mk	1.1"
#ident	"$Header: $"
#	Portions Copyright (c) 1988, Sun Microsystems, Inc.
#	All Rights Reserved


#	Makefile for pwck

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = pwck.mk

MAINS = pwck

OBJECTS =  pwck.o

SOURCES =  pwck.c

ALL:		$(MAINS)

$(MAINS):	pwck.o
		$(CC) -o pwck pwck.o $(LDFLAGS) $(PERFLIBS)

pwck.o:		 $(INC)/sys/types.h \
		 $(INC)/sys/param.h \
		 $(INC)/sys/fs/s5param.h \
		 $(INC)/sys/signal.h \
		 $(INC)/sys/sysmacros.h \
		 $(INC)/sys/stat.h \
		 $(INC)/stdio.h \
		 $(INC)/ctype.h 

GLOBALINCS = $(INC)/ctype.h \
	$(INC)/stdio.h \
	$(INC)/sys/fs/s5param.h \
	$(INC)/sys/param.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/types.h 


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) $(MAINS) 

