#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libw:port/makefile	1.1.4.1"
#ident "$Header: makefile 1.5 91/06/27 $"

include $(LIBRULES)

#
# makefile for libw/port
#
#
# The macro PROF is null by default, causing profiled object to be maintained.
# If profiled object is not desired, the reassignment PROF=@# should appear in
# the make command line.
#

.SUFFIXES: .p
PROF=
NONPROF=

all:
	$(MAKE) -f makefile $(MAKEARGS) prof
	$(MAKE) -f makefile $(MAKEARGS) nonprof

nonprof:
	$(NONPROF)cd wstdio; $(MAKE) -f makefile $(MAKEARGS) $(IGN) nonprof \
		PROF=@#
	$(NONPROF)cd wstring; $(MAKE) -f makefile $(MAKEARGS) $(IGN) nonprof \
		PROF=@#
	$(NONPROF)cd wmisc; $(MAKE) -f makefile $(MAKEARGS) $(IGN) nonprof \
		PROF=@#
	$(NONPROF)cd wctype; $(MAKE) -f makefile $(MAKEARGS) $(IGN) nonprof \
		PROF=@#
prof:
	$(PROF)cd wstdio; $(MAKE) -f makefile $(MAKEARGS) $(IGN) prof \
		NONPROF=@#
	$(PROF)cd wstring; $(MAKE) -f makefile $(MAKEARGS) $(IGN) prof \
		NONPROF=@#
	$(PROF)cd wmisc; $(MAKE) -f makefile $(MAKEARGS) $(IGN) prof \
		NONPROF=@#
	$(PROF)cd wctype; $(MAKE) -f makefile $(MAKEARGS) $(IGN) prof \
		NONPROF=@#

clean:

clobber: clean
	-rm -f */*.o
	-rm -f */*.p
