#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/HELP/Makefile	1.2"
#ident	"$Header: $"

include $(CMDRULES)

HMENUS=$(ROOT)/$(MACH)/usr/vmsys/HELP
HMENUDIRS=PS backup dskopts hwset logins mailset restore sched
HMENUFILES=T.h1.help T.h7.cmd T.hadmin
GRP=bin
OWN=bin

all:
	@echo "nothing to build in VMSYS/HELP."

install: all
	-[ -d $(HMENUS) ] || mkdir -p $(HMENUS)
	-for f in $(HMENUFILES) ; \
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(HMENUS) $$f ; \
	done

	-for dir in $(HMENUDIRS) ; \
	do \
		( cd $$dir ; $(MAKE) $(MAKEARGS) $@ ) ; \
	done

clean clobber size strip lintit:
	-for dir in $(HMENUDIRS) ; \
	do \
		( cd $$dir ; $(MAKE) $(MAKEARGS) $@ ) ; \
	done

