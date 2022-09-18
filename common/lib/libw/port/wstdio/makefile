#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)libw:port/wstdio/makefile	1.2.2.2"
#ident "$Header: makefile 1.4 91/06/27 $"

#
# makefile for libw/port/wstdio
#
#

include $(LIBRULES)

.SUFFIXES: .p
PROF=
NONPROF=
INCLIBW=../../inc
LOCALINC=-I$(INCLIBW)

OBJECTS=\
fgetwc.o    fgetws.o    fputwc.o     fputws.o	\
getwchar.o  getws.o     putwchar.o   putws.o	\
ungetwc.o

POBJECTS=\
fgetwc.p    fgetws.p    fputwc.p     fputws.p	\
getwchar.p  getws.p     putwchar.p   putws.p	\
ungetwc.p

nonprof: $(OBJECTS)

prof: $(POBJECTS)

fgetwc.o fgetwc.p fgetws.o fgetws.p \
	fputwc.o fputwc.p fputws.o fputws.p \
	getwchar.o getwchar.p getws.o getws.p \
	putwchar.o putwchar.p putws.o putws.p \
	ungetwc.o ungetwc.p : $(INC)/stdio.h
fgetws.o fgetws.p fputws.o fputws.p \
	getwchar.o getwchar.p getws.o getws.p \
	putwchar.o putwchar.p putws.o putws.p: $(INCLIBW)/widec.h
fgetwc.o fgetwc.p fputwc.o fputwc.p \
	ungetwc.o ungetwc.p: $(INCLIBW)/pcode.h
fgetwc.o fgetwc.p fgetws.o fgetws.p \
	fputwc.o fputwc.p fputws.o fputws.p \
	getwchar.o getwchar.p getws.o getws.p \
	putwchar.o putwchar.p putws.o putws.p \
	ungetwc.o ungetwc.p:
.c.o .c.p:
	$(NONPROF)@echo $*.c:
	$(NONPROF)$(CC) $(DEFLIST) $(CFLAGS) $(L16DEFS) -c $*.c
	$(PROF)@echo $*.c:
	$(PROF)$(CC) $(DEFLIST) $(CFLAGS) $(L16DEFS) -c -p $*.c && mv $(*F).o $*.p
