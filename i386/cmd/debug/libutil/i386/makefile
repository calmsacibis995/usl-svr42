#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# $Copyright: $
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.
#
#ident	"@(#)debugger:libutil/i386/makefile	1.5"

include $(CMDRULES)

TARGET = $(PRODLIB)/libutil.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/callstack.C \
	$(COMMON)/cancel_sig.C \
	$(COMMON)/change.C \
	$(COMMON)/create.C \
	$(COMMON)/curr_frame.C \
	$(COMMON)/curr_loc.C \
	$(COMMON)/destroy.C \
	$(COMMON)/dis.C \
	$(COMMON)/dump_raw.C \
	$(COMMON)/find_fcn.C \
	$(COMMON)/functions.C \
	$(COMMON)/get_addr.C \
	$(COMMON)/grab_core.C \
	$(COMMON)/grab_proc.C \
	$(COMMON)/inform.C \
	$(COMMON)/input.C \
	$(COMMON)/jump.C \
	$(COMMON)/language.C \
	$(COMMON)/list_src.C \
	$(COMMON)/print_map.C \
	$(COMMON)/print_expr.C \
	$(COMMON)/pfiles.C \
	$(COMMON)/print_path.C \
	$(COMMON)/pending.C \
	$(COMMON)/ps.C \
	$(COMMON)/regs.C \
	$(COMMON)/rel_proc.C \
	$(COMMON)/rename.C \
	$(COMMON)/run.C \
	$(COMMON)/set_fcn.C \
	$(COMMON)/set_path.C \
	$(COMMON)/set_onstop.C \
	$(COMMON)/set_sig.C \
	$(COMMON)/set_stop.C \
	$(COMMON)/set_sys.C \
	$(COMMON)/set_val.C \
	$(COMMON)/sstep.C \
	$(COMMON)/send_sig.C \
	$(COMMON)/stop.C \
	$(COMMON)/symbols.C \
	$(COMMON)/wait.C

CSOURCES=\
	$(COMMON)/debug_open.C

include DEPEND

include ../../util/common/lib.make
