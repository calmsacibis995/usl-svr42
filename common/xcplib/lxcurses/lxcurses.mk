#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcplxcurses:lxcurses.mk	1.1.3.1"
#ident  "$Header: lxcurses.mk 1.1 91/07/09 $"

include $(LIBRULES)

#
#	lxcurses.mk 1.1 90/03/30 lxcurses:lxcurses.mk
#

LGTXTNM	= _LIB_TEXT

LIBCUR = libxcurses.a
INCS = xcurses.h ext.h

LIBOBJS = addch.o addstr.o box.o clear.o clrtobot.o clrtoeol.o cr_put.o \
	  cr_tty.o curses.o delch.o deleteln.o delwin.o endwin.o erase.o \
	  getch.o getstr.o initscr.o insch.o insertln.o longname.o move.o \
	  mvprintw.o mvscanw.o mvwin.o newwin.o overlay.o overwrite.o \
	  printw.o refresh.o save_mode.o scanw.o scroll.o standout.o test.o \
	  touchwin.o tstp.o unctrl.o

.PRECIOUS: $(LIBCUR)

###
# standard targets
#
all: $(LIBCUR)

$(LIBCUR): $(LIBOBJS)
	$(AR) $(ARFLAGS) $(LIBCUR) $(LIBOBJS)

install: all
	$(INS) -f $(USRLIB) -m 0644 -u bin -g bin $(LIBCUR) 
	$(INS) -f $(ROOT)/$(MACH)/usr/include -m 0644 -u root -g sys xcurses.h

clean:
	rm -f $(LIBOBJS)

clobber: clean
	rm -f $(LIBCUR)

###
# extra targets
#
$(LIBOBJS): $(INCS)
