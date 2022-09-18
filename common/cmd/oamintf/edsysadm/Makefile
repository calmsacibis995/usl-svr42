#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/edsysadm/Makefile	1.10.7.2"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(CMDRULES)

## libraries used by this process
LINTLIBS = \
	$(USRLIB)/llib-ladm.ln ../libintf/llib-lintf.ln
LINTFLAGS = -u

## options used to build this command
INCDIR = ../intf_include
LOCALINC = -I $(INCDIR)
LDLIBS = -L ../libintf -lintf -ladm 

MAINS=\
	delsysadm getintdef getfld findmenu updt_pkgdesc getpkgdesc

O_SHFILES=\
	edsysadm mkpkg mkmf mkpf mkchgs test_menu valfiles edhelp

O_DFILES=\
	Help \
	Form.chgmenu Form.chgtask Form.collision Form.getpkg Form.menu \
	Form.save Form.savepkg Form.select Form.task Form.test \
	Form.overwrite Form.redo Form.rename \
	Menu.choices Menu.menu Menu.pkg Menu.task Menu.testmenu \
	Text.dups Text.ok Text.success Text.mkchgerr Text.tmerr \
	Text.gpderr Text.pkgerr \
	Text.direxist Text.namerr Text.nochange Text.noname Text.nopkg \
	Text.rename

LINTFILES=\
	updt_pkgdesc getpkgdesc findmenu getintdef getfld delsysadm

.MUTEX:	 all xinstall

all: $(O_SHFILES) $(MAINS)

getintdef: getintdef.o $(INCDIR)/intf.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	$(CH)chmod 775 $@

getfld: getfld.o
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	$(CH)chmod 775 $@

findmenu: findmenu.o $(INCDIR)/intf.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	$(CH)chmod 775 $@

delsysadm: delsysadm.o $(INCDIR)/intf.h $(INCDIR)/menu_io.h
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	$(CH)chmod 775 $@

getpkgdesc: getpkgdesc.o
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	$(CH)chmod 775 $@

updt_pkgdesc: updt_pkgdesc.o
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	$(CH)chmod 775 $@

clean:
	rm -f *.o

clobber: clean
	rm -f $(O_SHFILES) $(MAINS) lint.out

size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

lintit:
	rm -f lint.out
	for file in $(LINTFILES) ;\
	do \
		$(LINT) $(LINTFLAGS) $(DEFLIST) $$file.c $(LINTLIBS) >> lint.out ;\
	done

install: all xinstall

PROTO=Prototype
OBJECTS=$(MAINS) $(O_SHFILES) $(O_DFILES)
include ../Install
