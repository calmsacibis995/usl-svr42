#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rmount:rmount.mk	1.2.19.3"
#ident  "$Header: rmount.mk 1.5 91/06/28 $"

include $(CMDRULES)


OWN = root
GRP = sys

PROFILE =
DEBUG =
INSDIR = $(ETC)/rfs
LOCALDEF = $(DEBUG) $(PROFILE)
LDLIBS = -lns

EXECS = rmount rmnttry rumount

mainOBJS = \
	rmount.o \
	rmnttry.o \
	rumount.o

mainLN = $(mainOBJS:.o=.ln)

comOBJS = \
	mntlock.o \
	rd_rmnttab.o \
	wr_rmnttab.o \
	fqn.o \
	getrmntent.o \
	ismac.o

comLN = $(comOBJS:.o=.ln)

LINTFLAGS = -ux $(MORECPP) #-unx

FILES = $(mainOBJS) $(comOBJS)

SOURCES = $(FILES:.o=.c)
LNFILES	= $(FILES:.o=.ln)

.SUFFIXES: .ln

.c.ln :
	$(LINT) $(LINTFLAGS) -c $*.c

all: $(EXECS)

$(EXECS) : 	$$(@).o $(comOBJS)
	$(CC)	 $(@).o $(comOBJS) -o $(@) $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

lintit: 			$(mainLN) $(comLN)
	$(LINT) $(LINTFLAGS)	$(mainLN) $(comLN)

fqn.o: fqn.c \
	$(INC)/nserve.h \
	$(INC)/sys/types.h \
	$(INC)/sys/rf_sys.h \
	$(INC)/string.h \
	rmount.h

getrmntent.o: getrmntent.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	rmnttab.h \
	$(INC)/string.h

ismac.o: ismac.c \
	$(INC)/errno.h \
	$(INC)/sys/types.h \
	$(INC)/mac.h

mntlock.o: mntlock.c \
	$(INC)/sys/types.h \
	$(INC)/sys/errno.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/unistd.h \
	rmount.h

rd_rmnttab.o: rd_rmnttab.c \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/errno.h \
	$(INC)/stdio.h \
	rmount.h \
	rmnttab.h

rmnttry.o: rmnttry.c \
	$(INC)/stdio.h \
	$(INC)/sys/types.h \
	$(INC)/sys/stat.h \
	$(INC)/nserve.h \
	$(INC)/fcntl.h \
	$(INC)/signal.h \
	$(INC)/sys/mnttab.h \
	$(INC)/unistd.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/wait.h \
	rmount.h \
	rmnttab.h \
	$(INC)/errno.h \
	$(INC)/mac.h

rmount.o: rmount.c \
	$(INC)/sys/types.h \
	$(INC)/nserve.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/mnttab.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/mac.h \
	rmount.h \
	rmnttab.h

rumount.o: rumount.c \
	$(INC)/sys/types.h \
	$(INC)/nserve.h \
	$(INC)/sys/stat.h \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	$(INC)/sys/mnttab.h \
	$(INC)/unistd.h \
	$(INC)/string.h \
	rmount.h \
	rmnttab.h

wr_rmnttab.o: wr_rmnttab.c \
	$(INC)/sys/types.h \
	$(INC)/signal.h \
	$(INC)/stdio.h \
	$(INC)/time.h \
	$(INC)/limits.h \
	rmount.h \
	rmnttab.h

DIRS = $(INSDIR) $(USRNSERVE)

$(DIRS):
	[ -d $@ ] || mkdir -p $@

install: all $(DIRS)
	-rm -f $(USRNSERVE)/rmount
	-rm -f $(USRNSERVE)/rmnttry
	-rm -f $(USRNSERVE)/rumount
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) rmount
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) rmnttry
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) rumount
	-$(SYMLINK) /etc/rfs/rmount $(USRNSERVE)/rmount
	-$(SYMLINK) /etc/rfs/rmnttry $(USRNSERVE)/rmnttry
	-$(SYMLINK) /etc/rfs/rumount $(USRNSERVE)/rumount

uninstall:
	(cd $(INSDIR); rm -f $(EXECS))

clean:
	-rm -f *.o *.ln

clobber: clean
	-rm -f $(EXECS)
