#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:catalog.d/i386/makefile	1.8"

include $(CMDRULES)

AWK = awk
LIBINT = ../../libint
DEBUGINC = ../../inc
GUI = ../../gui.d

include ../../util/common/std.make

include DEPEND
TARGET = $(OBJECTS)

all:	$(OBJECTS) $(HELPFILES)

install:	$(OBJECTS) $(HELPFILES)
	-if [ ! -d $(USRLIB)/locale/C/MSGFILES ]; then \
		mkdir -p $(USRLIB)/locale/C/MSGFILES; \
	fi
	rm -f $(USRLIB)/locale/C/MSGFILES/debug.str
	rm -f $(USRLIB)/locale/C/MSGFILES/debug.ui.str
	cp Mcatalog $(USRLIB)/locale/C/MSGFILES/debug.str
	cp GMcatalog $(USRLIB)/locale/C/MSGFILES/debug.ui.str
	chmod 444 $(USRLIB)/locale/C/MSGFILES/debug.str
	chmod 444 $(USRLIB)/locale/C/MSGFILES/debug.ui.str
	-if [ ! -d $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug ]; then \
		mkdir -p $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug; \
	fi
	rm -f $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug/*
	cp $(HELPFILES) $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug
	chmod 444 $(ROOT)/$(MACH)/usr/X/lib/locale/C/help/debug/*
