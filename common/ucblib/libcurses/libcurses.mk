#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucblib/libcurses/libcurses.mk	1.1"
#ident	"$Header: $"

#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.


#
# makefile for libcurses.a
#
#

include $(LIBRULES)

ARFLAGS = q

PROF=
NONPROF=
INC1=$(ROOT)/$(MACH)/usr/ucbinclude
INCSYS=$(INC)/sys
INCSYS1=$(INC1)/sys
SDEFLIST=
INSDIR=$(ROOT)/$(MACH)/usr/ucblib
LOCALINC=-I. -I$(INC1)

OBJECTS = addch.o addstr.o box.o clear.o clrtobot.o clrtoeol.o cr_put.o \
	cr_tty.o curses.o delch.o deleteln.o delwin.o endwin.o erase.o \
	fullname.o getch.o getstr.o id_subwins.o idlok.o initscr.o insch.o \
	insertln.o longname.o move.o mvprintw.o mvscanw.o mvwin.o newwin.o \
	overlay.o overwrite.o printw.o putchar.o refresh.o scanw.o \
	scroll.o standout.o toucholap.o touchwin.o tstp.o unctrl.o

SOURCES = addch.c addstr.c box.c clear.c clrtobot.c clrtoeol.c cr_put.c \
	cr_tty.c curses.c delch.c deleteln.c delwin.c endwin.c erase.c \
	fullname.c getch.c getstr.c id_subwins.c idlok.c initscr.c insch.c \
	insertln.c longname.c move.c mvprintw.c mvscanw.c mvwin.c newwin.c \
	overlay.c overwrite.c printw.c putchar.c refresh.c scanw.c \
	scroll.c standout.c toucholap.c touchwin.c tstp.c unctrl.c


ALL:		 $(OBJECTS) libcurses.a

all:	ALL

addch.o: curses.ext curses.h

addstr.o: curses.ext curses.h

box.o: curses.ext curses.h

clear.o: curses.ext curses.h

clrtobot.o: curses.ext curses.h

clrtoeol.o: curses.ext curses.h

cr_put.o: curses.ext curses.h

cr_tty.o: curses.ext curses.h

curses.o: curses.h

delch.o: curses.ext curses.h

deleteln.o: curses.ext curses.h 

delwin.o: curses.ext curses.h 

endwin.o: curses.ext curses.h 

erase.o: curses.ext curses.h

fullname.o: curses.ext curses.h

getch.o: curses.ext curses.h

getstr.o: curses.ext curses.h

id_subwins.o: curses.ext curses.h

idlok.o: curses.ext curses.h 

initscr.o: curses.ext curses.h $(INC1)/signal.h

insch.o: curses.ext curses.h

insertln.o: curses.ext curses.h

longname.o:

move.o: curses.ext curses.h

mvprintw.o: curses.ext curses.h $(INC)/varargs.h

mvscanw.o: curses.ext curses.h $(INC)/varargs.h

mvwin.o: curses.ext curses.h

newwin.o: curses.ext curses.h

overlay.o: curses.ext curses.h $(INC)/ctype.h

overwrite.o: curses.ext curses.h $(INC)/ctype.h

printw.o: curses.ext curses.h $(INC)/varargs.h

putchar.o: curses.ext curses.h 

scanw.o: curses.ext curses.h $(INC)/varargs.h

scroll.o: curses.ext curses.h 

stdout.o: curses.ext curses.h 

toucholap.o: curses.ext curses.h 

touchwin.o: curses.ext curses.h 

tstp.o: curses.ext curses.h $(INC1)/signal.h

unctrl.o: curses.ext curses.h 

GLOBALINCS = ./curses.ext \
	./curses.h \
	$(INC)/ctype.h \
	$(INC1)/signal.h \
	$(INC)/varargs.h 

libcurses.a: 
	$(AR) $(ARFLAGS) libcurses.a `$(LORDER) *.o | $(TSORT)`

install: ALL
	$(INS) -f $(INSDIR) -m 644 libcurses.a

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) libcurses.a
