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
#ident	"@(#)debugger:debug.d/i386/makefile	1.1"

include $(CMDRULES)

BASENAME = debug

TARGET = ../../$(BASENAME)

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/docommands.C \
	$(COMMON)/do_assoc.C \
	$(COMMON)/main.C \
	$(COMMON)/handlers.C

CSOURCES = \
	$(COMMON)/sig_handle.c

$(TARGET):	$(PRODLIB)/libcmd.a    $(PRODLIB)/libint.a
$(TARGET):	$(PRODLIB)/libutil.a   $(PRODLIB)/libexecon.a
$(TARGET):	$(PRODLIB)/libexp.a    $(PRODLIB)/libmachine.a
$(TARGET):	$(PRODLIB)/libsymbol.a $(PRODLIB)/libint.a
$(TARGET):	$(PRODLIB)/libdbgen.a  $(PRODLIB)/libC.a
$(TARGET):	$(PRODLIB)/libedit.a

LIBRARIES = \
	$(PRODLIB)/libcmd.a \
	$(PRODLIB)/libexecon.a \
	$(PRODLIB)/libedit.a \
	$(PRODLIB)/libexp.a \
	$(PRODLIB)/libutil.a \
	$(PRODLIB)/libmachine.a \
	$(PRODLIB)/libsymbol.a \
	$(PRODLIB)/libint.a \
	$(PRODLIB)/libdbgen.a \
	-lelf \
	-lgen

include DEPEND

include ../../util/common/prog.make
