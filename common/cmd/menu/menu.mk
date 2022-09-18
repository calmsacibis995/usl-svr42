#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)menu.cmd:menu.mk	1.5"
#ident	"$Header: $"

include	$(CMDRULES)

MAINS	= menu
OBJECTS	= curses.o form.o main.o parse.o
SOURCES	= curses.c form.c main.c parse.c

all: $(MAINS)
	grep -v "^#" helphelp.sh > helphelp
	egrep -v "^#|^$$" menu.errs.sh > menu.errs

install: all
	[ -d $(ETC)/inst/locale/C/menus ] || mkdir -p $(ETC)/inst/locale/C/menus
	$(INS) -f $(USRSBIN) -m 555 -u bin -g bin menu
	$(INS) -f $(ETC)/inst/locale/C/menus -m 444 -u root -g other menu.errs
	$(INS) -f $(ETC)/inst/locale/C/menus -m 444 -u root -g other helphelp
	$(INS) -f $(ETC)/inst/locale/C/menus -m 444 -u root -g other menu_colors.sh

menu:	$(OBJECTS)
	$(CC) -o menu $(OBJECTS) $(LDFLAGS) -lform -lcurses

curses.o:	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/curses.h \
	$(INC)/form.h \
	$(INC)/errno.h \
	$(INC)/signal.h \
	$(INC)/string.h \
	$(INC)/sys/kd.h \
	$(INC)/sys/termios.h \
	menu.h

form.o:	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/curses.h \
	$(INC)/form.h \
	$(INC)/errno.h \
	$(INC)/signal.h \
	$(INC)/string.h \
	menu.h

main.o:	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/curses.h \
	$(INC)/form.h \
	$(INC)/errno.h \
	$(INC)/signal.h \
	$(INC)/string.h \
	menu.h

parse.o:	$(INC)/stdlib.h \
	$(INC)/stdio.h \
	$(INC)/fcntl.h \
	$(INC)/curses.h \
	$(INC)/form.h \
	$(INC)/errno.h \
	$(INC)/signal.h \
	$(INC)/string.h \
	menu.h

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)
