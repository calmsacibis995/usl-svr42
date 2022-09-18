#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:libexecon/i386/makefile	1.1"

include $(CMDRULES)

TARGET = $(PRODLIB)/libexecon.a

include ../../util/common/std.make

CCSOURCES = \
	Proccore.C \
	Frame.C \
	Reg1.C \
	RegAccess.C \
	Seg_mach.C \
	HW_Watch.C \
	oldproc.C \
	$(COMMON)/Breaklist.C \
	$(COMMON)/EventTable.C \
	$(COMMON)/Event.C \
	$(COMMON)/Ev_Notify.C \
	$(COMMON)/LWP.check.C \
	$(COMMON)/LWP.ctl.C \
	$(COMMON)/LWP.event.C \
	$(COMMON)/LWP.io.C \
	$(COMMON)/LWP.new.C \
	$(COMMON)/LWP.resp.C \
	$(COMMON)/LWP.sym.C \
	$(COMMON)/Process.C \
	$(COMMON)/Procctl.C \
	$(COMMON)/Proglist.C \
	$(COMMON)/Program.C \
	$(COMMON)/PtyList.C \
	$(COMMON)/Reg.C \
	$(COMMON)/Seglist.C \
	$(COMMON)/Segment.C \
	$(COMMON)/Siglist.C \
	$(COMMON)/StopEvent.C \
	$(COMMON)/TSClist.C

CSOURCES=\
	Coffcore.c

include DEPEND

include ../../util/common/lib.make
