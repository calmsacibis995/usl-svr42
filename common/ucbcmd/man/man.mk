#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/man/man.mk	1.2"
#ident	"$Header: $"


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
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#	Makefile for man/whatis/apropos/catman

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

LIBDIR = $(ROOT)/$(MACH)/usr/ucblib

OWN = root

GRP = bin

MAKEFILE = man.mk

MAINS = man whatis apropos catman getNAME makewhatis

OBJECTS =  man.o catman.o getNAME.o

SOURCES =  man.c catman.c getNAME.c

ALL:		$(MAINS)

man: man.o
	$(CC) -o man  man.o $(LDFLAGS) $(PERFLIBS)

whatis:	man
	-$(RM) -f whatis
	@ln man whatis

apropos: man
	-$(RM) -f apropos
	@ln man apropos

catman: catman.o
	$(CC) -o catman  catman.o $(LDFLAGS) $(PERFLIBS)

getNAME: getNAME.o
	$(CC) -o getNAME  getNAME.o $(LDFLAGS) $(PERFLIBS)

makewhatis: makewhatis.sh
	cp makewhatis.sh makewhatis

man.o:	$(INC)/stdio.h $(INC)/ctype.h $(INC)/sys/types.h \
	$(INC)/sys/param.h $(INC)/sgtty.h $(INC)/sys/stat.h \
	$(INC)/signal.h $(INC)/string.h 

catman.o:$(INC)/stdio.h $(INC)/ctype.h $(INC)/sys/types.h \
	$(INC)/sys/param.h $(INC)/errno.h $(INC)/sys/stat.h \
	$(INC)/dirent.h $(INC)/sys/time.h 

getNAME.o:$(INC)/stdio.h $(INC)/string.h 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -m 555 -u $(OWN) -g $(GRP) -f $(INSDIR) man 
	-rm -f $(INSDIR)/whatis $(INSDIR)/apropos
	-$(RM) -f $(INSDIR)/whatis $(INSDIR)/apropos
	ln $(INSDIR)/man $(INSDIR)/whatis
	ln $(INSDIR)/man $(INSDIR)/apropos
	$(INS) -m 555 -u $(OWN) -g $(GRP) -f $(INSDIR) catman
	$(INS) -m 555 -u $(OWN) -g $(GRP) -f $(LIBDIR) getNAME
	$(INS) -m 555 -u $(OWN) -g $(GRP) -f $(LIBDIR) makewhatis

