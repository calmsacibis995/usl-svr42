#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)su:su.mk	1.9.10.4"
#ident  "$Header: su.mk 1.3 91/06/24 $"

include $(CMDRULES)

#	Copyright (c) 1987, 1988 Microsoft Corporation	
#	  All Rights Reserved	
#	This Module contains Proprietary Information of Microsoft  
#	Corporation and should be treated as Confidential.	   
#	su make file

OWN = root
GRP = sys

LDLIBS = -lcrypt_i -lcmd -liaf -lgen

all: su

su: su.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS) 

su.o: su.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/crypt.h \
	$(INC)/time.h \
	$(INC)/signal.h \
	$(INC)/fcntl.h \
	$(INC)/string.h \
	$(INC)/ia.h \
	$(INC)/mac.h \
	$(INC)/deflt.h \
	$(INC)/libgen.h \
	$(INC)/errno.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	$(INC)/sys/secsys.h

install: all
	-rm -f $(USRBIN)/su
	$(INS) -f $(SBIN) -m 04555 -u $(OWN) -g $(GRP) su
	$(SYMLINK) /sbin/su $(USRBIN)/su
	-mkdir ./tmp
	-$(CP) su.dfl ./tmp/su
	$(INS) -f $(ETC)/default -m 0444 -u $(OWN) -g $(GRP) ./tmp/su
	-rm -rf ./tmp

clean:
	rm -f su.o

clobber: clean
	rm -f su

lintit:
	$(LINT) $(LINTFLAGS) su.c
