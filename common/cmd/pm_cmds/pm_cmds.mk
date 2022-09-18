#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)pm_cmds:pm_cmds.mk	1.9.3.3"
#ident  "$Header: pm_cmds.mk 1.5 91/07/02 $"

include $(CMDRULES)


OWN = sys
GRP = priv

DEFDIR = $(ROOT)/$(MACH)`grep DEFLT $(INC)/deflt.h | sed -e 's/.*"\//\//' -e 's/"//'`
LDLIBS = -lcmd

MAINS = filepriv initprivs

OBJECTS = pmlib.o filepriv.o initprivs.o

SOURCES = $(OBJECTS:.o=.c)

all: $(MAINS)

filepriv: filepriv.o pmlib.o 
	$(CC) -o $@ filepriv.o pmlib.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

initprivs: initprivs.o pmlib.o
	$(CC) -o $@ initprivs.o pmlib.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

filepriv.o: filepriv.c \
	pdf.h \
	$(INC)/pfmt.h \
	$(INC)/priv.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/limits.h \
	$(INC)/locale.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/time.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/resource.h

initprivs.o: initprivs.c \
	pdf.h \
	$(INC)/pfmt.h \
	$(INC)/priv.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/limits.h \
	$(INC)/locale.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/time.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/resource.h

pmlib.o: pmlib.c \
	pdf.h \
	$(INC)/pfmt.h \
	$(INC)/priv.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/stdio.h \
	$(INC)/limits.h \
	$(INC)/locale.h \
	$(INC)/signal.h $(INC)/sys/signal.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/sys/mac.h \
	$(INC)/sys/time.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/types.h \
	$(INC)/sys/param.h \
	$(INC)/sys/secsys.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/resource.h

install: all
	$(INS) -f $(SBIN) -m 0555 -u $(OWN) -g $(GRP) filepriv
	$(INS) -f $(SBIN) -m 0550 -u $(OWN) -g $(GRP) initprivs
	-mkdir ./tmp
	-$(CP) privcmds.dfl ./tmp/privcmds
	$(INS) -f $(DEFDIR) -m 0444 -u root -g sys ./tmp/privcmds
	-rm -rf ./tmp

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

#	These targets are useful but optional

partslist:
	@echo $(MAKEFILE) $(SOURCES) $(LOCALINCS) | tr ' ' '\012' | sort

productdir:
	@echo  | tr ' ' '\012' | sort

product:
	@echo $(MAINS) | tr ' ' '\012' | \
	sed 's;^;/;'

srcaudit:
	@fileaudit $(MAKEFILE) $(LOCALINCS) $(SOURCES) -o $(OBJECTS) $(MAINS)
