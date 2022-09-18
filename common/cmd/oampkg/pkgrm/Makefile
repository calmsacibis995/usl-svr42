#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)oampkg:common/cmd/oampkg/pkgrm/Makefile	1.8.8.2"
#ident "$Header: Makefile 1.2 91/04/10 $"

include $(CMDRULES)

SRCBASE=../
PROC=pkgrm
OBJECTS=$(PROC)

## libraries used by this process
LIBINST=../libinst/libinst.a
LINTLIBS=\
	$(USRLIB)/llib-lpkg.ln $(USRLIB)/llib-ladm.ln \
	../libinst/llib-linst.ln

## options used to build this command
LOCALINC=-I. -I ../hdrs
LDLIBS=-L ../libinst -linst -lpkg -ladm -lcmd

## objects which make up this process
OFILES=\
	main.o quit.o presvr4.o

## C files to be linted
LINTFILES=\
	main.c quit.c presvr4.c 

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
