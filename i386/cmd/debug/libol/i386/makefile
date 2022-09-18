#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:libol/i386/makefile	1.4"

include $(CMDRULES)

TARGET = $(PRODLIB)/libol.a

include ../../util/common/std.make

GINCLIST = -I../../gui.d/common -I../../gui.d/$(CPU) -I../common \
	-I$(ROOT)/$(MACH)/usr/X/include

XDEFS = -DUSL=1 -DSVR4=1
CPLUS_CMD = $(CPLUS) $(XDEFS) $(GINCLIST) $(CPLUS_CMD_FLAGS)
CC_CMD = $(CC) $(XDEFS) $(GINCLIST) $(CC_CMD_FLAGS)

CCSOURCES = \
	$(COMMON)/Alert_sh.C \
	$(COMMON)/Boxes.C \
	$(COMMON)/Caption.C \
	$(COMMON)/Dialog_sh.C \
	$(COMMON)/Menu.C \
	$(COMMON)/Radio.C \
	$(COMMON)/Sel_list.C \
	$(COMMON)/Stext.C \
	$(COMMON)/Table.C \
	$(COMMON)/Text_area.C \
	$(COMMON)/Text_disp.C \
	$(COMMON)/Text_line.C \
	$(COMMON)/Toggle.C \
	$(COMMON)/Window_sh.C \
	$(COMMON)/callbacks.C \
	$(COMMON)/olutil.C \
	$(COMMON)/xpm.c

include DEPEND

include ../../util/common/lib.make
