#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:libedit/i386/makefile	1.1"

include $(CMDRULES)

TARGET = $(PRODLIB)/libedit.a

include ../../util/common/std.make

CSOURCES = \
	../common/edit.c \
	../common/editlib.c \
	../common/emacs.c \
	../common/history.c \
	../common/vi.c \
	../common/defs.c

CCSOURCES = \
	../common/ksh_fc.C \
	../common/debug_read.C

include DEPEND

include ../../util/common/lib.make
