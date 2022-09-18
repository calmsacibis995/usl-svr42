#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)priocntl:i386/cmd/priocntl/priocntl.mk	1.4.9.6"
#ident  "$Header: priocntl.mk 1.4 91/06/28 $"

include $(CMDRULES)

CLASSDIR = $(USRLIB)/class
DIRS	 = \
	$(CLASSDIR)	\
	$(CLASSDIR)/RT	\
	$(CLASSDIR)/TS	\
	$(CLASSDIR)/FC	\
	$(CLASSDIR)/VC
LDLIBS = -lgen

SOURCES = priocntl.c rtpriocntl.c subr.c tspriocntl.c vcpriocntl.c fcpriocntl.c
OBJECTS = $(SOURCES:.c=.o)

all: priocntl classes

priocntl: priocntl.o subr.o
	$(CC) priocntl.o subr.o -o priocntl $(LDFLAGS) $(LDLIBS) $(SHLIBS)

classes: RTpriocntl TSpriocntl VCpriocntl FCpriocntl

RTpriocntl: rtpriocntl.o subr.o
	$(CC) rtpriocntl.o subr.o -o RTpriocntl $(LDFLAGS) $(LDLIBS) $(SHLIBS)

TSpriocntl: tspriocntl.o subr.o
	$(CC) tspriocntl.o subr.o -o TSpriocntl $(LDFLAGS) $(LDLIBS) $(SHLIBS)

FCpriocntl: fcpriocntl.o subr.o
	$(CC) fcpriocntl.o subr.o -o FCpriocntl $(LDFLAGS) $(LDLIBS) $(SHLIBS)

VCpriocntl: vcpriocntl.o subr.o
	$(CC) vcpriocntl.o subr.o -o VCpriocntl $(LDFLAGS) $(LDLIBS) $(SHLIBS)

priocntl.o: priocntl.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/string.h \
	$(INC)/search.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/dirent.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/procfs.h \
	$(INC)/macros.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h \
	priocntl.h

rtpriocntl.o: rtpriocntl.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/rtpriocntl.h \
	$(INC)/sys/param.h \
	$(INC)/sys/hrtcntl.h \
	$(INC)/limits.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h \
	$(INC)/sys/secsys.h \
	priocntl.h

tspriocntl.o: tspriocntl.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/tspriocntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h \
	$(INC)/sys/secsys.h \
	priocntl.h

fcpriocntl.o: fcpriocntl.c \
	$(INC)/stdio.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/priocntl.h \
	$(INC)/sys/fcpriocntl.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/priv.h \
	$(INC)/sys/secsys.h \
	priocntl.h

vcpriocntl.o: vcpriocntl.c \
	$(INC)/stdio.h	\
	$(INC)/unistd.h	\
	$(INC)/sys/unistd.h	\
	$(INC)/sys/types.h	\
	$(INC)/sys/select.h	\
	$(INC)/sys/types.h	\
	$(INC)/sys/procset.h	\
	$(INC)/sys/priocntl.h	\
	$(INC)/sys/vcpriocntl.h	\
	$(INC)/errno.h	\
	$(INC)/sys/errno.h	\
	./priocntl.h

subr.o: subr.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/unistd.h \
	$(INC)/sys/types.h \
	$(INC)/sys/procset.h \
	$(INC)/sys/priocntl.h \
	priocntl.h

$(DIRS):
	-mkdir -p $@

install: all $(DIRS)
	$(INS) -f $(USRBIN) -u root -g root -m 4555 priocntl
	$(INS) -f $(CLASSDIR)/RT -u bin -g bin -m 555 RTpriocntl
	$(INS) -f $(CLASSDIR)/TS -u bin -g bin -m 555 TSpriocntl
	$(INS) -f $(CLASSDIR)/VC -u bin -g bin -m 555 VCpriocntl
	$(INS) -f $(CLASSDIR)/FC -u bin -g bin -m 555 FCpriocntl

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f priocntl RTpriocntl TSpriocntl VCpriocntl FCpriocntl

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)
