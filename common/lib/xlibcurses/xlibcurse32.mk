#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)curses:common/lib/xlibcurses/xlibcurse32.mk	1.1.2.5"
#ident "$Header: xlibcurse32.mk 1.4 91/06/27 $"
#
#	Curses Library High Level Makefile.
#
#	To INSTALL libcurses32.a, the tic compiler and the tools type:
#
#		"make install"
#
#
#	To COMPILE libcurses32.a, the tic compiler and the tools, type:
#
#		"make all"
#
#
#	To compile a particular file with normal compilation type:
#
#		"make FILES='<particular .o files>"
#
#
#	If debugging is desired then type:
#
#		"make O=debug FILES='<particular .o files>"
#
#
#	If tracing is desired then type:
#
#		"make O=trace FILES='<particular .o files>"
#
#
#	If profiling is desired then type:
#
#		"make O=profile FILES='<particular .o files>"
#
#
#	To compile only the tic compiler type:
#
#		"make tic"
#
#
#	To create cpio files for all directories type:
#		"make cpio"
#
#

include $(LIBRULES)

SRCD=./screen

all:
	@cd $(SRCD) ; $(MAKE) -f makefile.32 rmhdrs $(MAKEARGS)
	@cd $(SRCD) ; $(MAKE) -f makefile.32 cktmp $(MAKEARGS)
	@cd $(SRCD) ; $(MAKE) -f makefile.32 $(MAKEARGS)

libcurses32.a:
	#@cd $(SRCD) ; $(MAKE) -f makefile.32 rmhdrs $(MAKEARGS)
	#@cd $(SRCD) ; $(MAKE) -f makefile.32 cktmp $(MAKEARGS)
	@cd $(SRCD) ; $(MAKE) -f makefile.32 libcurses32.a $(MAKEARGS)
	@echo
	@echo "libcurses32.a has been made."
	@echo

tools:
	@cd $(SRCD) ; $(MAKE) -f makefile.32 tools $(MAKEARGS)
	@cd $(SRCD) ; $(MAKE) -f makefile.32 llib-lcurses.ln $(MAKEARGS)
	@echo
	@echo "Libcurses/Terminfo tools have been made."
	@echo

tic:
	@cd $(SRCD) ; $(MAKE) -f makefile.32 tic $(MAKEARGS)
	@echo
	@echo "The tic compiler has been made."
	@echo
	
install:
	# make and install libcurses32.a and tic
	@cd $(SRCD) ; $(MAKE) -f makefile.32 cktmp $(MAKEARGS)
	@cd $(SRCD) ; $(MAKE) -f makefile.32 install $(MAKEARGS)
	@echo
	@echo libcurses32.a, the tic compiler, and associated tools have
	@echo been installed.
	@echo
	@if [ "$(CCSBIN)" = "$(TOOLS)/usr/ccs/bin" ]; \
	then \
		cd $(SRCD) ; $(MAKE) -f makefile.32 CC='$(HCC)' INC='/usr/include' ticclob tic ; \
                $(RM) -f $(CCSBIN)/captoinfo $(CCSBIN)/infocmp $(CCSBIN)/tput ;\
	else \
		cd $(SRCD) ; $(MAKE) -f makefile.32 ticclob tic ; \
	fi
	$(INS) -f $(CCSBIN)  -m 755 screen/tic

clean:
	@cd $(SRCD) ; $(MAKE) -f makefile.32 clean $(MAKEARGS)

clobber:
	@cd $(SRCD) ; $(MAKE) -f makefile.32 clobber $(MAKEARGS)

cpio:
	@echo
	@/bin/echo "\n\tBuilding cpio files in ${HOME}\n\n\t\c"
	@find . -print|cpio -ocud|split -10000 - ${HOME}/crs.`date +%m%d`.
	@/bin/echo "\n\tcpio files have been built\n"

bsd:
	@echo
	cd $(SRCD); mv makefile makefile.sysv; cp makefile.bsd makefile
	cd $(SRCD); make rmident
	@echo "Curses has been converted for BSD"
# this has only been tested on 4.2BSD, but we assume libc has getopt.

