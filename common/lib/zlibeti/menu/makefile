#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libeti:menu/makefile	1.15.2.4"
#

include $(LIBRULES)

PWD=.
LIBRARY = libmenu.a
PRODUCT = $(LIBRARY) llib-lmenu.ln llib-lmenu

SOURCES=global.c newmenu.c newitem.c menuformat.c menumark.c menuopts.c\
        menupad.c scale.c menuwin.c menusub.c show.c affect.c itemvalue.c\
        driver.c link.c post.c itemopts.c chk.c menucursor.c\
        pattern.c visible.c terminit.c menuitems.c\
        connect.c curitem.c topitem.c menufore.c menuback.c menugrey.c\
        menuserptr.c itemusrptr.c itemcount.c

OBJ = \
	affect.o chk.o connect.o curitem.o driver.o \
	global.o itemcount.o itemopts.o itemusrptr.o \
	itemvalue.o link.o menuback.o \
	menucursor.o menufore.o menuformat.o menugrey.o \
	menuitems.o menumark.o menuopts.o menupad.o \
	menuserptr.o menusub.o menuwin.o newitem.o \
	newmenu.o pattern.o post.o scale.o show.o terminit.o \
	topitem.o visible.o

GLOBALINC =
LOCALINC = -I$(PWD) -I$(PWD)/.. -I$(INC)

HFILES=private.h menu.h

.PRECIOUS:	$(LIBRARY)

all:		$(PRODUCT)

$(LIBRARY):	$(OBJ)
	$(AR) $(ARFLAGS) $(LIBRARY) `$(LORDER) $(OBJ) | $(TSORT)`

affect.o: affect.c \
	./private.h

chk.o: chk.c \
	./private.h

connect.o: connect.c \
	./private.h

curitem.o: curitem.c \
	./private.h

driver.o: driver.c \
	./private.h

global.o: global.c \
	./menu.h

itemcount.o: itemcount.c \
	./private.h

itemopts.o: itemopts.c \
	./private.h

itemusrptr.o: itemusrptr.c \
	./private.h

itemvalue.o: itemvalue.c \
	./private.h

link.o: link.c \
	./private.h

llib-lmenu.o: llib-lmenu.c \
	./menu.h

menuback.o: menuback.c \
	./private.h

menucursor.o: menucursor.c \
	./private.h

menufore.o: menufore.c \
	./private.h

menuformat.o: menuformat.c \
	./private.h

menugrey.o: menugrey.c \
	./private.h

menuitems.o: menuitems.c \
	./private.h

menumark.o: menumark.c \
	./private.h

menuopts.o: menuopts.c \
	./private.h

menupad.o: menupad.c \
	./private.h \
	$(INC)/ctype.h

menuserptr.o: menuserptr.c \
	./private.h

menusub.o: menusub.c \
	./private.h

menuwin.o: menuwin.c \
	./private.h

newitem.o: newitem.c \
	./private.h

newmenu.o: newmenu.c \
	./private.h

pattern.o: pattern.c \
	./private.h \
	$(INC)/ctype.h

post.o: post.c \
	./private.h

scale.o: scale.c \
	./private.h

show.o: show.c \
	./private.h

terminit.o: terminit.c \
	./private.h

topitem.o: topitem.c \
	./private.h

visible.o: visible.c \
	./private.h


lintit:
		$(LINT) $(LOCALINC) $(LINTFLAGS) -l curses $(SOURCES)

llib-lmenu:	llib-lmenu.c
		$(CP) llib-lmenu.c llib-lmenu

llib-lmenu.ln:	llib-lmenu.c
		$(LINT) -o menu -vx $(LOCALINC) llib-lmenu.c

install:	all
		install -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) libmenu.a
		install -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) llib-lmenu
		install -f $(CCSLIB) -m 644 -u $(OWN) -g $(GRP) llib-lmenu.ln
		install -f $(ROOT)/$(MACH)/usr/include -m 444 -u $(OWN) -g $(GRP) menu.h

clean:
		rm -f *.o

clobber:	clean
		rm -f $(PRODUCT)
