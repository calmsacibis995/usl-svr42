#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lprof:lprof.mk	1.10.1.13"

include $(CMDRULES)

include lprofinc.mk

all:  basicblk cmds 

basicblk:
	cd bblk/$(CPU); $(MAKE) -f bblk.mk

cmds:
	if test "$(NATIVE)" = "yes"; then \
		cd cmd; $(MAKE) -f cmd.mk ; \
	fi

install: all
	$(MAKE) target -f lprof.mk LPTARGET=install

lintit:
	$(MAKE) target -f lprof.mk LPTARGET=lintit

clean:
	$(MAKE) target -f lprof.mk LPTARGET=clean

clobber: clean
	$(MAKE) target -f lprof.mk LPTARGET=clobber

target:
	cd bblk/$(CPU); \
	$(MAKE) -f bblk.mk $(LPTARGET); \
	cd ../..; 
	if test "$(NATIVE)" = "yes"; then \
	   cd cmd; \
	   $(MAKE) -f cmd.mk $(LPTARGET) ; \
	   cd ..; \
	   cd libprof/$(CPU); \
	   $(MAKE) -f libprof.mk $(LPTARGET) ; \
	   cd ../..; \
	fi
