#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oampkg:common/cmd/oampkg/pkgmk/Makefile	1.8.10.2"
#ident "$Header: $"

include $(CMDRULES)

SRCBASE=../
PROC=pkgmk
OBJECTS=$(PROC)

## libraries used by this process
LIBINST=../libinst/libinst.a
LINTLIBS=\
	$(USRLIB)/llib-lpkg.ln $(USRLIB)/llib-ladm.ln \
	../libinst/llib-linst.ln

## options used to build this command
LOCALINC=-I ../hdrs
LDLIBS=-L ../libinst -linst -lpkg -ladm -lcmd -lgen

## objects which make up this process
OFILES=\
	splpkgmap.o main.o quit.o mkpkgmap.o getinst.o

## C files to be linted
LINTFILES=\
	splpkgmap.c main.c quit.c mkpkgmap.c getinst.c

all:	$(PROC)

$(PROC): $(OFILES) $(LIBINST)
	$(CC) -o $(PROC) $(OFILES) $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	chmod 775 $(PROC)

clean:
	rm -f $(OFILES)

clobber: clean
	rm -f $(PROC)

lintit:
	$(LINT) $(DEFLIST) $(LINTFILES) $(LINTLIBS) >lint.out 2>&1

include $(SRCBASE)Install
