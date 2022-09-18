#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)tfm:tfm.mk	1.7.2.2"
#ident "$Header: tfm.mk 1.4 91/04/29 $"

include $(CMDRULES)

#	Copyright (c) 1988 AT&T
#	  All Rights Reserved
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Makefile for tfm

LDLIBS = -lcmd

MAINS  = adminrole adminuser tfadmin

SOURCES = tfmlib.c privlib.c adminrole.c adminuser.c tfadmin.c err.c

OBJECTS = $(SOURCES:.c=.o)
LDOBJS  = tfmlib.o privlib.o err.o

all: $(MAINS)

adminrole: adminrole.o $(LDOBJS) 
	$(CC) -o $@ $@.o $(LDOBJS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

adminuser: adminuser.o $(LDOBJS)
	$(CC) -o $@ $@.o $(LDOBJS) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

tfadmin: tfadmin.o $(LDOBJS)
	$(CC) -o $@ $@.o $(LDOBJS) $(LDFLAGS) $(LDLIBS) -lgen $(ROOTLIBS)

tfmlib.o: tfmlib.c \
	$(INC)/ctype.h \
	$(INC)/priv.h \
	$(INC)/sys/secsys.h \
	$(INC)/dirent.h \
	$(INC)/fcntl.h \
	$(INC)/unistd.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/limits.h \
	$(INC)/stdio.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h \
	$(INC)/sys/time.h \
	$(INC)/mac.h \
	$(INC)/pfmt.h \
	err.h \
	tfm.h

privlib.o: privlib.c \
	$(INC)/priv.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/stdio.h \
	$(INC)/limits.h \
	$(INC)/pfmt.h \
	err.h

err.o: err.c \
	$(INC)/stdio.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	err.h

adminrole.o: adminrole.c \
	$(INC)/limits.h \
	$(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	err.h \
	tfm.h

adminuser.o: adminuser.c \
	$(INC)/limits.h \
	$(INC)/stdio.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	err.h \
	tfm.h

tfadmin.o: tfadmin.c \
	$(INC)/sys/types.h \
	$(INC)/limits.h \
	$(INC)/stdio.h \
	$(INC)/pwd.h \
	$(INC)/memory.h \
	$(INC)/unistd.h \
	$(INC)/priv.h \
	$(INC)/locale.h \
	$(INC)/pfmt.h \
	err.h \
	tfm.h \
	$(INC)/sys/param.h \
	$(INC)/audit.h \
	$(INC)/libgen.h \
	$(INC)/string.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

install: all
	$(INS) -f $(USRBIN) -m 0700 -u root -g sys adminrole
	$(INS) -f $(USRBIN) -m 0700 -u root -g sys adminuser
	$(INS) -f $(SBIN) -m 0555 -u bin -g bin tfadmin

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo $(USRBIN) | tr ' ' '\012' | sort

product:
	@echo $(MAINS) | tr ' ' '\012' | \
	sed 's;^;$(USRBIN)/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
