#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libeti:panel/makefile	1.12.2.4"
#

include $(LIBRULES)

PWD=.
LIBRARY = libpanel.a
PRODUCT = $(LIBRARY) llib-lpanel.ln llib-lpanel

GLOBALINC =
LOCALINC = -I$(PWD) -I$(INC)

SOURCES = bottom.c	delete.c	misc.c	update.c	\
	  move.c	new.c		top.c	replace.c

OBJ = \
	bottom.o delete.o misc.o move.o \
	new.o replace.o top.o update.o

.PRECIOUS:	${LIBRARY}

all:	$(PRODUCT)

$(LIBRARY):	$(OBJ)
	$(AR) $(ARFLAGS) $(LIBRARY) `$(LORDER) $(OBJ) | $(TSORT)`

bottom.o: bottom.c \
	$(INC)/curses.h \
	./private.h

delete.o: delete.c \
	$(INC)/curses.h \
	./private.h

misc.o: misc.c \
	$(INC)/curses.h \
	./private.h

move.o: move.c \
	$(INC)/curses.h \
	./private.h

new.o: new.c \
	$(INC)/curses.h \
	./private.h

replace.o: replace.c \
	$(INC)/curses.h \
	./private.h

top.o: top.c \
	$(INC)/curses.h \
	./private.h

update.o: update.c \
	$(INC)/curses.h \
	./private.h

lintit:
	$(LINT) $(LOCALINC) $(LINTFLAGS) -l curses $(SOURCES)

llib-lpanel:	llib-lpanel.c
		$(CP) llib-lpanel.c llib-lpanel

llib-lpanel.ln:	llib-lpanel.c
		$(LINT) -o panel -vx $(LOCALINC) llib-lpanel.c

install:	all
		install -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) libpanel.a
		install -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) llib-lpanel
		install -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) llib-lpanel.ln
		install -f $(ROOT)/$(MACH)/usr/include -m 444 -u $(OWN) -g $(GRP) panel.h

clean:
		rm -f *.o

clobber:	clean
		rm -f $(PRODUCT)
