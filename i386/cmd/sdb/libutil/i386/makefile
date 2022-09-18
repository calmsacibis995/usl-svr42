#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sdb:libutil/i386/makefile	1.7"

include $(CMDRULES)

TARGET = $(PRODLIB)/libutil.a

include ../../util/common/std.make

COMMON=../common

CCSOURCES = \
	$(COMMON)/assoc.C \
	$(COMMON)/callstack.C \
	$(CPU)/change_pc.C \
	$(COMMON)/create.C \
	$(COMMON)/curr_src.C \
	$(COMMON)/destroy.C \
	$(COMMON)/disassem.C \
	$(COMMON)/disp_break.C \
	$(COMMON)/eval_addr.C \
	$(COMMON)/eval_expr.C \
	$(COMMON)/find_fcn.C \
	$(COMMON)/first_line.C \
	$(COMMON)/get_addr.C \
	$(COMMON)/grab_core.C \
	$(COMMON)/grab_proc.C \
	$(COMMON)/grabbed.C \
	$(COMMON)/inform.C \
	$(COMMON)/istep.C \
	$(COMMON)/istepover.C \
	$(COMMON)/num_lines.C \
	$(COMMON)/pc_curr.C \
	$(COMMON)/print_map.C \
	$(COMMON)/regs.C \
	$(COMMON)/rel_proc.C \
	$(COMMON)/rem_break.C \
	$(COMMON)/rem_watch.C \
	$(COMMON)/remallbk.C \
	$(COMMON)/remallwp.C \
	$(COMMON)/run.C \
	$(COMMON)/set_break.C \
	$(COMMON)/set_path.C \
	$(COMMON)/set_sig.C \
	$(COMMON)/set_stmt.C \
	$(COMMON)/set_watch.C \
	$(COMMON)/src_text.C \
	$(COMMON)/sstep.C \
	$(COMMON)/sstepover.C \
	$(COMMON)/sufxpath.C \
	$(CPU)/watch_step.C

include DEPEND

include ../../util/common/lib.make
