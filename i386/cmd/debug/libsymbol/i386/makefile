#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

#ident	"@(#)debugger:libsymbol/i386/makefile	1.1"

include $(CMDRULES)

TARGET = $(PRODLIB)/libsymbol.a

include ../../util/common/std.make

CCSOURCES = \
	$(COMMON)/AddrList.C \
	$(COMMON)/Build.C \
	$(COMMON)/Coff.C \
	$(COMMON)/Coffbuild.C \
	$(COMMON)/Dwarfbuild.C \
	$(COMMON)/ELF.C \
	$(COMMON)/Elfbuild.C \
	$(COMMON)/Evaluator.C \
	$(COMMON)/Protoline.C \
	$(COMMON)/Protorec.C \
	$(COMMON)/Locdesc.C \
	$(COMMON)/NameList.C \
	$(COMMON)/Object.C \
	$(COMMON)/Reflist.C \
	$(COMMON)/Source.C \
	$(COMMON)/Symbol.C \
	$(COMMON)/Symtab.C \
	$(COMMON)/Symtable.C \
	$(COMMON)/TYPE.C \
	TYPE.Mach.C \
	Cvtaddr.C \
	Coff_Bit.C \
	get_stype.C

include DEPEND

include ../../util/common/lib.make
