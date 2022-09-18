#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
#ident	"@(#)libeti:zlibeti.mk	1.7.2.3"
#

include $(LIBRULES)

all:
		@echo "\nMaking all for menu\n"
		cd menu; $(MAKE) all
		@echo "\nMaking all for form\n"
		cd form; $(MAKE) all
		@echo "\nMaking all for panel\n"
		cd panel; $(MAKE) all

install:
		@echo "\nMaking install for menu\n"
		cd menu; $(MAKE) install
		@echo "\nMaking install for form\n"
		cd form; $(MAKE) install
		@echo "\nMaking install for panel\n"
		cd panel; $(MAKE) install
		install -f $(ROOT)/$(MACH)/usr/include -m 444 -u $(OWN) -g $(GRP) eti.h

lintit:		
		@echo "\nMaking lintit for menu\n"
		cd menu; $(MAKE) lintit
		@echo "\nMaking lintit for form\n"
		cd form; $(MAKE) lintit
		@echo "\nMaking lintit for panel\n"
		cd panel; $(MAKE) lintit

clean:		
		@echo "\nMaking clean for menu\n"
		cd menu; $(MAKE) clean
		@echo "\nMaking clean for form\n"
		cd form; $(MAKE) clean
		@echo "\nMaking clean for panel\n"
		cd panel; $(MAKE) clean

clobber:	clean
		@echo "\nMaking clobber for menu\n"
		cd menu; $(MAKE) clobber
		@echo "\nMaking clobber for form\n"
		cd form; $(MAKE) clobber
		@echo "\nMaking clobber for panel\n"
		cd panel; $(MAKE) clobber
