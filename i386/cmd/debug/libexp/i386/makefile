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

#ident	"@(#)debugger:libexp/i386/makefile	1.5"

include $(CMDRULES)

TARGET = $(PRODLIB)/libexp.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/CCeval.C \
	CCevalMach.C \
	$(COMMON)/CCconvert.C \
	$(COMMON)/CCgram.Y \
	$(COMMON)/CClalex.C \
	$(COMMON)/CCresolve.C \
	$(COMMON)/CCtlex.C \
	$(COMMON)/Const.C \
	$(COMMON)/Debug_var.C \
	$(COMMON)/Dbgvarsupp.C \
	$(COMMON)/CCtree.C \
	$(COMMON)/Expr.C \
	$(COMMON)/ParsedRep.C \
	$(COMMON)/print_rval.C \
	$(COMMON)/Resolver.C \
	$(COMMON)/Rvalue.C \
	$(COMMON)/Value.C \
	$(COMMON)/lex_util.C \
	$(COMMON)/type.C\
	Val_BitF.C \
	cvt_util.C 

CSOURCES=\
	$(SGSBASE)/fpemu/common/fpemu.c

include DEPEND

include ../../util/common/lib.make
