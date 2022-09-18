#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sdb:libexecon/i386/makefile	1.5"

include $(CMDRULES)

TARGET = $(PRODLIB)/libexecon.a

include ../../util/common/std.make

CCSOURCES = \
	Core.C \
	Frame.C \
	Reg1.C \
	DbregAccess.C \
	RegAccess.C \
	Seglist.C \
	Segment.C \
	oslevel.C \
	$(COMMON)/Assoccmds.C \
	$(COMMON)/Breaklist.C \
	$(COMMON)/Circlech.C \
	$(COMMON)/Cmdstream.C \
	$(COMMON)/EventTable.C \
	$(COMMON)/Monitor.s.C \
	$(COMMON)/Process.hx.C \
	$(COMMON)/Process.lx.C \
	$(COMMON)/Process.ev.C \
	$(COMMON)/Process.st.C \
	$(COMMON)/Process.io.C \
	$(COMMON)/Reg.C \
	$(COMMON)/Siglist.C \
	$(COMMON)/Sigset.C \
	$(COMMON)/Status.s.C \
	$(COMMON)/TSClist.C \
	$(COMMON)/Watchlist.C \
	$(COMMON)/Watch2.C

include DEPEND

include ../../util/common/lib.make
