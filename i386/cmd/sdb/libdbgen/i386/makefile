#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sdb:libdbgen/i386/makefile	1.4"

include $(CMDRULES)

TARGET = $(PRODLIB)/libdbgen.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/Avltree.C \
	$(COMMON)/Link.C \
	$(COMMON)/Vector.C \
	$(COMMON)/Wordstack.C \
	$(COMMON)/str.C

include DEPEND

include ../../util/common/lib.make
