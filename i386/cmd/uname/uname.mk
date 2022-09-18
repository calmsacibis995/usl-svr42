#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)uname:i386/cmd/uname/uname.mk	1.4.10.2"
#ident "$Header: uname.mk 1.2 91/03/14 $"

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

#	Makefile for uname 

OWN = bin
GRP = bin
OBJECTS=uname.o scoinfo.o

all: uname

uname: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

uname.o: uname.c \
	$(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/errno.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/utsname.h \
	$(INC)/sys/systeminfo.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/sys/fcntl.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysi86.h 

scoinfo.o: scoinfo.s
	$(AS) scoinfo.s

install: all
	$(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) uname
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) uname

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f uname

lintit:
	$(LINT) $(LINTFLAGS) uname.c

#	These targets are useful but optional

partslist:
	@echo uname.mk uname.c scoinfo.s $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo uname | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit uname.mk $(LOCALINCS) uname.c scoinfo.s -o uname.o uname
