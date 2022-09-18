#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:debugsrc.mk	1.6"

#
#	Note: we use our make command even when using
#	other host tools.  We know our make will support
#	includes when used with macros, parallel make, etc.
#

include $(CMDRULES)
include util/common/defs.make


UI = ol

LIBDIRS = libcmd libdbgen libedit libexecon libexp libint libmachine libsymbol libutil libC lib$(UI)


CLI = debug follow

GUI = debug.$(UI).ui

PRODUCTS = $(CLI) $(GUI)

DEBUGARGS= UI='$(UI)'

DIRS = $(LIBDIRS) debug.d follow.d gui.d tutorial.d

UTILS= lib cfront/cfront util/common/munch

CLLIBS = lib/libcmd.a lib/libdbgen.a lib/libedit.a lib/libexecon.a \
	lib/libexp.a lib/libint.a \
	lib/libmachine.a lib/libsymbol.a lib/libutil.a

GUILIBS = lib/libdbgen.a lib/libint.a lib/lib$(UI).a

FORCE = force

.MUTEX:		$(UTILS) cltargets guitargets catalog.d/$(CPU)

all:	$(PRODUCTS)

cli:	$(CLI)

gui:	$(GUI)

lib:
		mkdir lib

cfront/cfront:
		chmod +x util/common/depend util/common/Basename
		chmod +x util/common/mkdefine util/common/substdir
		sed -e '1,$$s/..PFX./$(PFX)/g' <util/common/CC >util/$(CPU)/CC
		chmod 755 util/$(CPU)/CC
		cd cfront ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

util/common/munch:	cfront/munch
		cp cfront/munch util/common/munch

cltargets:	$(CLLIBS) lib/libC.a

guitargets:	$(GUILIBS) lib/libC.a

alias:	debug_alias

debug:	$(UTILS) catalog.d/$(CPU) cltargets $(FORCE)
		cd debug.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

follow:
	cd follow.d/$(CPU); $(MAKE) $(MAKEARGS) $(DEBUGARGS)

debug.$(UI).ui: $(UTILS) catalog.d/$(CPU) guitargets $(FORCE)
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

lib/libC.a:
		cd libC/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

lib/libcmd.a:	$(FORCE)
	cd libcmd/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

lib/libdbgen.a:	$(FORCE)
		cd libdbgen/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

lib/libedit.a:	$(FORCE)
	cd libedit/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

lib/libexecon.a:	$(FORCE)
		cd libexecon/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

lib/libexp.a:	$(FORCE)
		cd libexp/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

lib/libint.a:	$(FORCE)
		cd libint/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

lib/libmachine.a:	$(FORCE)
		cd libmachine/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

lib/libsymbol.a:	$(FORCE)
		cd libsymbol/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

lib/libutil.a:	$(FORCE)
		cd libutil/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

lib/lib$(UI).a:	$(FORCE)
	cd lib$(UI)/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

catalog.d/$(CPU):	$(FORCE)
	cd catalog.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

tutorial.d/$(CPU):	$(FORCE)
	cd tutorial.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS)

install_cli:	cli
	cd debug.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cd follow.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS) install
	cd catalog.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cd tutorial.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cp debug_alias $(CCSLIB)/debug_alias

install_gui:	gui
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS) install

install:	all
	cd debug.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cd follow.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS) install
	cd gui.d/$(CPU) ; $(MAKE) $(MAKEARGS) $(DEBUGARGS) install
	cd catalog.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cd tutorial.d/$(CPU) ; $(MAKE) install $(MAKEARGS) $(DEBUGARGS)
	cp debug_alias $(CCSLIB)/debug_alias


depend:	lib catalog.d/$(CPU)
	@for i in $(DIRS) ;\
	do echo $$i: ; ( cd $$i/$(CPU) ; $(MAKE) depend $(DEBUGARGS) ) ;\
	done

clean:	
	@for i in $(DIRS) ;\
	do echo $$i: ; ( cd $$i/$(CPU) ; $(MAKE) clean $(DEBUGARGS) ) ;\
	done
	cd cfront ; $(MAKE) clean
	cd catalog.d/$(CPU) ; $(MAKE) clean

clobber: 
	@for i in $(DIRS) ;\
	do echo $$i: ; ( cd $$i/$(CPU) ; $(MAKE) clobber $(DEBUGARGS) ) ;\
	done
	cd cfront ; $(MAKE) clobber
	cd catalog.d/$(CPU) ; $(MAKE) clobber
	rm -f util/common/munch lib/libC.a

lintit:
	@echo "can't lint C++"

rebuild:	clobber depend all

force:
	@:
