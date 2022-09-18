#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)gcore:common/cmd/gcore/gcore.mk	1.2.5.3"
#ident  "$Header: gcore.mk 1.3 91/06/27 $"

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
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#	Makefile for gcore

OWN = bin
GRP = bin

all: gcore

gcore: gcore.o
	$(CC) -o gcore gcore.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

gcore.o: gcore.c \
	$(INC)/stdio.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/ctype.h \
	$(INC)/string.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/fstyp.h \
	$(INC)/sys/fsid.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/file.h \
	$(INC)/sys/immu.h \
	$(INC)/sys/user.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/procfs.h \
	$(INC)/sys/elf.h \
	$(INC)/sys/elf_386.h \
	$(INC)/sys/mman.h

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) gcore 

clean:
	rm -f gcore.o

clobber: clean
	rm -f gcore

lintit:
	$(LINT) $(LINTFLAGS) gcore.c

#	These targets are useful but optional

partslist:
	@echo gcore.mk gcore.c $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo gcore | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit gcore.mk $(LOCALINCS) gcore.c -o gcore.o gcore
