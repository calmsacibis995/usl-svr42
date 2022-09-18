#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)prof:prof.mk	1.8.7.6"

include $(CMDRULES)

PROF_SAVE	=

CMDBASE		= ..
SGSBASE		= ../sgs
LPBASE		= $(SGSBASE)/lprof
PLBBASE		= $(LPBASE)/libprof

LIBELF		= -lelf
PROFLIBD	= $(PLBBASE)
LIBSYMINT	= -L$(PROFLIBD) -lsymint
LDFLAGS		= -s
LIBS		= $(LIBSYMINT) $(LIBELF)

ENVPARMS	= \
	CMDRULES="$(CMDRULES)" PROF_SAVE="$(PROF_SAVE)" CMDBASE="$(CMDBASE)" \
	SGSBASE="$(SGSBASE)" LPBASE="$(LPBASE)" PLBASE="$(PLBASE)" \
	LIBELF="$(LIBELF)" PROFILBD="$(PROFLIBD)" LIBSYMINT="$(LIBSYMINT)" \
	LDFLAGS="$(LDFLAGS)" LIBS="$(LIBS)"

all:
	cd $(CPU) ; $(MAKE) all $(ENVPARMS)

install:
	cd $(CPU) ; $(MAKE) install

lintit:
	cd $(CPU) ; $(MAKE) lintit $(ENVPARMS)

clean:
	cd $(CPU) ; $(MAKE) clean

clobber:
	cd $(CPU) ; $(MAKE) clobber
