#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sdb:libsymbol/i386/makefile	1.7"

include $(CMDRULES)

TARGET = $(PRODLIB)/libsymbol.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/AddrList.C \
	$(COMMON)/Bdi.C \
	$(COMMON)/Bdibuild.C \
	$(COMMON)/Cache.C \
	$(COMMON)/Coff.C \
	$(COMMON)/Coffbuild.C \
	$(COMMON)/Elfbuild.C \
	$(COMMON)/Evaluator.C \
	$(COMMON)/Fetalline.C \
	$(COMMON)/Fetalrec.C \
	$(COMMON)/Locdesc.C \
	$(COMMON)/NameList.C \
	$(COMMON)/Object.C \
	$(COMMON)/Reflist.C \
	$(COMMON)/SectHdr.C \
	$(COMMON)/Source.C \
	$(COMMON)/Symbol.C \
	$(COMMON)/Symtab.C \
	$(COMMON)/Symtable.C \
	$(COMMON)/TYPE.C \
	$(COMMON)/Tagcheck.C \
	$(COMMON)/builder.C \
	$(COMMON)/dbtools.C \
	$(COMMON)/dbtools1.C \
	$(CPU)/reg_no.C

include DEPEND

include ../../util/common/lib.make
