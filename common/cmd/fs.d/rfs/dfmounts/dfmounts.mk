#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)rfs.cmds:rfs/dfmounts/dfmounts.mk	1.5.8.3"
#ident	"$Header: dfmounts.mk 1.3 91/06/28 $"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/rfs
OWN = bin
GRP = bin
INCSYS = $(INC)
FRC =
LINTFLAGS = -ux #-unx

.SUFFIXES: .o .ln

.c.ln :
	$(LINT) $(DEFLIST) $(LINTOPTS)	-c $*.c

all: dfmounts

dfmounts: dfmounts.o
	$(CC) -o $@ dfmounts.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

dfmounts.o : dfmounts.c  \
	$(INC)/stdio.h \
	$(INCSYS)/sys/fcntl.h \
	$(INCSYS)/sys/types.h \
	$(INCSYS)/sys/nserve.h \
	$(INCSYS)/sys/stat.h \
	$(INCSYS)/sys/rf_sys.h

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) dfmounts

clean:
	rm -f dfmounts.o dfmounts.ln

clobber: clean
	rm -f dfmounts

lintit: dfmounts.ln
	$(LINT) $(LINTFLAGS) dfmounts.ln
FRC:

