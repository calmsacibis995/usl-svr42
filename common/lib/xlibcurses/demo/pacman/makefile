#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)curses:common/lib/xlibcurses/demo/pacman/makefile	1.3.2.2"
#ident "$Header: makefile 1.4 91/06/27 $"

include $(LIBRULES)
CFILES =	pacman.c monster.c util.c movie.c
OFILES =	pacman.o monster.o util.o movie.o

LDLIBS=-lcurses

pacman:	$(OFILES)
	$(CC) $(CFLAGS) $(DEFLIST) $(LDFLAGS) -o pacman $(OFILES) $(LDLIBS) $(SHLIBS)

pacman.o:	pacman.c pacdefs.h

monster.o:	monster.c pacdefs.h

util.o:	util.c pacdefs.h

movie.o:	movie.c pacdefs.h

install:	pacman
	$(INS) -f $(ROOT)/$(MACH)/usr/games pacman

strip:	pacman
	strip pacman

shrink:
	-rm -f *.o

clean:	shrink
	-rm -f pacman errs core a.out

lintit:	$(CFILES)
	$(LINT) -pc $(CFILES)

list:	$(CFILES) pacdefs.h makefile
	oprl -x makefile $(CFILES) pacdefs.h
	oprl -x -C $(CFILES)
	touch list
