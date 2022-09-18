#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:libcmd/i386/makefile	1.1"

include $(CMDRULES)

TARGET = $(PRODLIB)/libcmd.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/Execute.C \
	$(COMMON)/Help.C \
	$(COMMON)/Input.C \
	$(COMMON)/Keyword.C \
	$(COMMON)/Location.C \
	$(COMMON)/Parser.C \
	$(COMMON)/Scanner.C \
	$(COMMON)/Shell.C \
	$(COMMON)/systable.C \
	sysmach.C

include DEPEND

include ../../util/common/lib.make
