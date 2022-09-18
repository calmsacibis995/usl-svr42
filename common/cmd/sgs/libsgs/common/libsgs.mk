#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sgs:libsgs/common/libsgs.mk	1.11"
#
#  makefile for libsgs.a
#
#
include $(CMDRULES)

.SUFFIXES: .p .P .A
HOSTCC=cc
NONPROF=
DEFLIST=
SDEFLIST=
INCLIST=-I$(CPUINC)
HOSTAWK=awk
MAC=
HOSTAR=ar

OBJECTS=\
errlst.o	gettxt.o	mbstowcs.o	mbtowc.o	\
new_list.o	pfmt.o		qsort.o		setcat.o	\
setlabel.o	setlocale.o	strerror.o	strtoul.o	\
wcstombs.o	wctomb.o

.MUTEX:	setup build archive install
all:	setup build archive install

setup:
	rm -rf _wchar.h errlist errlist.awk mbstowcs.c mbtowc.c	\
		qsort.c synonyms.h strtoul.c wcstombs.c wctomb.c
	cp ../../../../lib/libc/inc/synonyms.h synonyms.h
	cp ../../../../lib/libc/port/gen/_wchar.h _wchar.h
	cp ../../../../lib/libc/port/gen/errlist errlist
	cp ../../../../lib/libc/port/gen/errlist.awk errlist.awk
	cp ../../../../lib/libc/port/gen/mbstowcs.c mbstowcs.c
	cp ../../../../lib/libc/port/gen/mbtowc.c mbtowc.c
	cp ../../../../lib/libc/port/gen/qsort.c qsort.c
	cp ../../../../lib/libc/port/gen/strtoul.c strtoul.c
	cp ../../../../lib/libc/port/gen/wcstombs.c wcstombs.c
	cp ../../../../lib/libc/port/gen/wctomb.c wctomb.c

build: $(OBJECTS)

.c.o .c.p .c.P .c.A:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(HOSTCC) $(DEFLIST) $(SDEFLIST) $(INCLIST) $(CFLAGS) -c $*.c

.MUTEX:	new_list.c errlst.c
new_list.c errlst.c: errlist errlist.awk
	$(HOSTAWK) -f errlist.awk <errlist

archive:
	#
	# build the archive with the modules in correct order.
	$(HOSTAR) q lib.libsgs `lorder *.o | tsort`

install:
	mv lib.libsgs ../libsgs.a

clean:
	-rm -f *.o objlist lib.libsgs

clobber: clean
	rm -rf uxsyserr errlst.c new_list.c ../libsgs.a _wchar.h	\
		errlist errlist.awk mbstowcs.c mbtowc.c	qsort.c		\
		synonyms.h strtoul.c wcstombs.c wctomb.c
