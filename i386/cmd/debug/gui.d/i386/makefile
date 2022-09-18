#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:gui.d/i386/makefile	1.7"

include $(CMDRULES)

BASENAME = debug.$(UI).ui

TARGET = ../../$(BASENAME)

include ../../util/common/std.make

DEFLIST = -I../../lib$(UI)/common -I$(ROOT)/$(MACH)/usr/X/include

XDEFS = -DUSL=1 -DSVR4=1
XLIBPATH = /usr/X/lib
CPLUS_CMD = $(CPLUS) $(XDEFS) $(CPLUS_CMD_FLAGS)
CC_CMD = $(CC) $(XDEFS) $(CC_CMD_FLAGS)

CCSOURCES = \
	$(COMMON)/main.C \
	$(COMMON)/Windows.C \
	$(COMMON)/Command.C \
	$(COMMON)/Context.C \
	$(COMMON)/Ps_pane.C \
	$(COMMON)/Stack_pane.C \
	$(COMMON)/Syms_pane.C \
	$(COMMON)/Events.C \
	$(COMMON)/Source.C \
	$(COMMON)/Sch_dlg.C \
	$(COMMON)/Status.C \
	$(COMMON)/Dis.C \
	$(COMMON)/Dispatcher.C \
	$(COMMON)/Dialogs.C \
	$(COMMON)/Ctrl_dlg.C \
	$(COMMON)/Event_dlg.C \
	$(COMMON)/Help.C \
	$(COMMON)/Proclist.C \
	$(COMMON)/Eventlist.C \
	$(COMMON)/Notifier.C \
	$(COMMON)/util.C

include DEPEND

$(TARGET):	$(PRODLIB)/libint.a
$(TARGET):	$(PRODLIB)/libdbgen.a
$(TARGET):	../../lib/lib$(UI).a

include ../../lib$(UI)/$(CPU)/ui.libs

all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	LD_RUN_PATH=$(XLIBPATH) $(CPLUS) -o $(TARGET) $(LINK_MODE) $(OBJECTS) $(LIBRARIES) $(LDLIBS)

install:	$(CCSLIB)/$(BASENAME) $(UI)install

$(CCSLIB)/$(BASENAME):	$(TARGET)
	$(STRIP) $(TARGET)
	cp $(TARGET) $(CCSLIB)/$(BASENAME)
