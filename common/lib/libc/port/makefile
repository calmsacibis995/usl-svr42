#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libc-port:makefile	1.17.6.2"
#
# makefile for libc/port
#
#
# The macro PROF is null by default, causing profiled object to be maintained.
# If profiled object is not desired, the reassignment PROF=@# should appear in
# the make command line.
#

include $(LIBRULES)

.SUFFIXES: .p .P .A
PROF=
NONPROF=
PIC=
ABI=
SDEFLIST=

.MUTEX:	prof pic abi nonprof

all:	prof pic abi nonprof

nonprof:
	$(NONPROF)cd gen; $(MAKE) $(IGN) nonprof PROF=@# PIC=@# ABI=@#
	$(NONPROF)cd print; $(MAKE) $(IGN) nonprof PROF=@# PIC=@# ABI=@#
	$(NONPROF)cd stdio; $(MAKE) $(IGN) nonprof PROF=@# PIC=@# ABI=@#
	$(NONPROF)cd sys; $(MAKE) $(IGN) nonprof PROF=@# PIC=@# ABI=@#
prof:
	$(PROF)cd gen; $(MAKE) $(IGN) prof NONPROF=@# PIC=@# ABI=@#
	$(PROF)cd print; $(MAKE) $(IGN) prof NONPROF=@# PIC=@# ABI=@#
	$(PROF)cd stdio; $(MAKE) $(IGN) prof NONPROF=@# PIC=@# ABI=@#
	$(PROF)cd sys; $(MAKE) $(IGN) prof NONPROF=@# PIC=@# ABI=@#
pic:
	$(PIC)cd gen; $(MAKE) $(IGN) pic PROF=@# NONPROF=@# ABI=@#
	$(PIC)cd print; $(MAKE) $(IGN) pic PROF=@# NONPROF=@# ABI=@#
	$(PIC)cd stdio; $(MAKE) $(IGN) pic PROF=@# NONPROF=@# ABI=@#
	$(PIC)cd sys; $(MAKE) $(IGN) pic PROF=@# NONPROF=@# ABI=@#
abi:
	$(ABI)cd gen; $(MAKE) $(IGN) abi PROF=@# NONPROF=@# PIC=@#
	$(ABI)cd print; $(MAKE) $(IGN) abi PROF=@# NONPROF=@# PIC=@#
	$(ABI)cd stdio; $(MAKE) $(IGN) abi PROF=@# NONPROF=@# PIC=@#
	$(ABI)cd sys; $(MAKE) $(IGN) abi PROF=@# NONPROF=@# PIC=@#
	
clean:

clobber: clean
	-find . \( -name '*.o' -o -name '*.p' -o -name '*.P' -o -name '*.A' \) \
		-print | xargs rm -f
	-rm -f gen/new_list.c
	-rm -f gen/errlst.c
