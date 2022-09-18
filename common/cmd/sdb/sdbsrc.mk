#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sdb:sdbsrc.mk	1.4"

#
#	Note: we use our make command even when using
#	other host tools.  We know our make will support
#	includes when used with macros, parallel make, etc.
#

include $(CMDRULES)
include util/common/defs.make

LIBDIRS = libdbgen libexecon libexp libint libmachine libsymbol libutil libC

PRODUCTS = sdb

DIRS = $(LIBDIRS) sdb.d

UTILS= lib cfront/cfront util/common/munch

LIBS = lib/libdbgen.a lib/libexecon.a lib/libexp.a lib/libint.a \
	lib/libmachine.a lib/libsymbol.a lib/libutil.a

TARGETS= $(LIBS) lib/libC.a

FORCE = force

.MUTEX:		$(UTILS) targets

all:	$(PRODUCTS)

$(PRODUCTS):	$(UTILS) targets $(FORCE)
		cd sdb.d/$(CPU) ; $(MAKE) YFLAGS=-ld $(MAKEARGS)

lib:
		mkdir lib

cfront/cfront:
		chmod +x util/common/depend util/common/Basename
		chmod +x util/common/mkdefine util/common/substdir
		sed -e '1,$$s/..PFX./$(CPU)/g' <util/common/CC >util/$(CPU)/CC
		chmod 755 util/$(CPU)/CC
		cd cfront ; $(MAKE) $(MAKEARGS)

util/common/munch:	cfront/munch
		cp cfront/munch util/common/munch

targets:	$(TARGETS)

lib/libC.a:
		cd libC/$(CPU) ; $(MAKE) $(MAKEARGS)

lib/libdbgen.a:	$(FORCE)
		cd libdbgen/$(CPU) ; $(MAKE) $(MAKEARGS)

lib/libexecon.a:	$(FORCE)
		cd libexecon/$(CPU) ; $(MAKE) $(MAKEARGS)

lib/libexp.a:	$(FORCE)
		cd libexp/$(CPU) ; $(MAKE) $(MAKEARGS)

lib/libint.a:	$(FORCE)
		cd libint/$(CPU) ; $(MAKE) $(MAKEARGS)

lib/libmachine.a:	$(FORCE)
		cd libmachine/$(CPU) ; $(MAKE) $(MAKEARGS)

lib/libsymbol.a:	$(FORCE)
		cd libsymbol/$(CPU) ; $(MAKE) $(MAKEARGS)

lib/libutil.a:	$(FORCE)
		cd libutil/$(CPU) ; $(MAKE) $(MAKEARGS)

install:	sdb
		cd sdb.d/$(CPU) ; $(MAKE) install $(MAKEARGS)

depend:	lib
	@for i in $(DIRS) ;\
	do echo $$i: ; ( cd $$i/$(CPU) ; $(MAKE) depend ) ;\
	done

clean:	
	@for i in $(DIRS) ;\
	do echo $$i: ; ( cd $$i/$(CPU) ; $(MAKE) clean ) ;\
	done
	cd cfront ; $(MAKE) clean
	rm -f sdb.d/$(CPU)/y.tab.h

clobber: 
	@for i in $(DIRS) ;\
	do echo $$i: ; ( cd $$i/$(CPU) ; $(MAKE) clobber ) ;\
	done
	cd cfront ; $(MAKE) clobber
	rm -f util/common/munch lib/libC.a sdb.d/$(CPU)/y.tab.h

lintit:
	@echo "can't lint C++"

rebuild:	clobber depend all

force:
	@:
