#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/libintf/Makefile	1.4.7.2"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(LIBRULES)
.c.a:;

PIC = 
LIBINTF = libintf.a
LINTFLAGS = -u
LINTLIBINTF = llib-lintf.ln
LINTFILES = \
	find_menu.c save_old.c read_item.c write_item.c rec_pkg.c \
	sys_calls.c menu_io.c inst_err.c iexpr.c menutok.c
INCDIR = ../intf_include
LOCALINC = -I $(INCDIR)

LIBFILES= \
	$(LIBINTF)(find_menu.o) $(LIBINTF)(save_old.o) $(LIBINTF)(read_item.o) \
	$(LIBINTF)(write_item.o) $(LIBINTF)(rec_pkg.o) $(LIBINTF)(sys_calls.o) \
	$(LIBINTF)(menu_io.o) $(LIBINTF)(inst_err.o) $(LIBINTF)(iexpr.o) \
	$(LIBINTF)(menutok.o)

all:	$(LIBINTF)
	: $(LIBINTF) is up to date

.PRECIOUS: $(LIBINTF)

$(LIBINTF): $(LIBFILES) 
	$(CC) -c $(CFLAGS) $(PIC) $(DEFLIST) $(?:.o=.c) $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	$(AR) $(ARFLAGS) $(LIBINTF) $?
	rm $?

$(LIBINTF)(find_menu.o):	$(INCDIR)/intf.h $(INCDIR)/menu_io.h
$(LIBINTF)(save_old.o):		$(INCDIR)/intf.h
$(LIBINTF)(read_item.o):	$(INCDIR)/intf.h
$(LIBINTF)(write_item.o):	$(INCDIR)/intf.h
$(LIBINTF)(rec_pkg.o):		$(INCDIR)/intf.h
$(LIBINTF)(sys_calls.o):	$(INCDIR)/intf.h
$(LIBINTF)(iexpr.o):		$(INCDIR)/intf.h
$(LIBINTF)(inst_err.o):		$(INCDIR)/inst_err.h
$(LIBINTF)(menu_io.o):		$(INCDIR)/intf.h $(INCDIR)/menu_io.h \
				$(INCDIR)/inst_err.h

clean:
	rm -f lint.out llib-libintf.ln llib-lintf.ln *.o

clobber: clean
	rm -f $(LIBINTF)

strip:
	: nothing to strip

install: all
	: nothing to install

lintit:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -o intf $(LINTFILES) > lint.out 2>&1
