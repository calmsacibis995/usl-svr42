#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oampkg:common/cmd/oampkg/pkginstall/Makefile	1.9.10.2"
#ident "$Header: $"

include $(CMDRULES)

SRCBASE=../
PROC=pkginstall
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
	quit.o check.o dockspace.o \
	cppath.o getbase.o getinst.o merginfo.o \
	reqexec.o instvol.o main.o backup.o \
	sortmap.o pkgvolume.o predepend.o pkgenv.o

## C files to be linted
LINTFILES=\
	quit.c check.c dockspace.c \
	cppath.c getbase.c getinst.c merginfo.c \
	reqexec.c instvol.c main.c backup.c \
	sortmap.c pkgvolume.c predepend.c pkgenv.c

all:	$(PROC)

$(PROC): $(OFILES) $(LIBINST)
	$(CC) -o $(PROC) $(OFILES) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)
	chmod 775 $(PROC)

clean:
	rm -f $(OFILES)

clobber: clean
	rm -f $(PROC)

lintit:
	$(LINT) $(DEFLIST) $(LINTFILES) $(LINTLIBS) >lint.out 2>&1

include $(SRCBASE)Install
