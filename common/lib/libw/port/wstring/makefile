#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libw:port/wstring/makefile	1.2.2.2"
#ident "$Header: makefile 1.4 91/06/27 $"

#
# makefile for libw/port/wstring
#
#

include $(LIBRULES)

.SUFFIXES: .p
PROF=
NONPROF=
INCLIBW= ../../inc
LOCALINC=-I$(INCLIBW)

OBJECTS=\
strtows.o   wscat.o     wschr.o     wscmp.o	\
wscpy.o     wscspn.o    wslen.o     wsncat.o	\
wsncmp.o    wsncpy.o    wsntostr.o  wspbrk.o	\
wsrchr.o    wssize.o    wsspn.o     wstok.o	\
wstostr.o   wsntostr.o

POBJECTS=\
strtows.p   wscat.p     wschr.p     wscmp.p	\
wscpy.p     wscspn.p    wslen.p     wsncat.p	\
wsncmp.p    wsncpy.p    wsntostr.p  wspbrk.p	\
wsrchr.p    wssize.p    wsspn.p     wstok.p	\
wstostr.p   wsntostr.p

nonprof: $(OBJECTS)

prof: $(POBJECTS)

.c.o .c.p:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(CFLAGS) $(L16DEFS) -c $*.c
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(CFLAGS) $(L16DEFS) -c -p $*.c && mv $(*F).o $*.p
strtows.o strtows.p \
	wscat.o wscat.p wschr.o wschr.p	\
	wscmp.o wscmp.p wscpy.o wscpy.p \
	wscspn.o wscspn.p wslen.o wslen.p \
	wsncat.o wsncat.p wsncmp.o wsncmp.p \
	wsncpy.o wsncpy.p wsntostr.o wsntostr.p \
	wspbrk.o wspbrk.p wsrchr.o wsrchr.p \
	wssize.o wssize.p wsspn.o wsspn.p \
	wstok.o wstok.p \
	wstostr.o wstostr.p: $(INCLIBW)/widec.h
strtows.o strtows.p wsntostr.o wsntostr.p \
	wstostr.o wstostr.p \
	wssize.o wssize.p: $(INCLIBW)/pcode.h
strtows.o wssize.o \
	wsntostr.o wstostr.o: 
