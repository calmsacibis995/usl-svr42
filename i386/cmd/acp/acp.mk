#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1987, 1988 Microsoft Corporation
#	  All Rights Reserved

#	This Module contains Proprietary Information of Microsoft 
#	Corporation and should be treated as Confidential.

#ident	"@(#)acp:acp.mk	1.2.1.5"

include	$(CMDRULES)

COMMANDS = fsck mount
FRC =
EVENT = $(USRLIB)/event

all:
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk $(MAKEARGS) all); \
	done

install: $(EVENT)
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk $(MAKEARGS) install); \
	done
	$(INS) -f $(EVENT) -m 644 -u bin -g bin devices
	$(INS) -f $(EVENT) -m 644 -u bin -g bin ttys
	$(INS) -f $(ETC) -m 644 -u bin -g bin socket.conf

$(EVENT):
	-mkdir -p $@
	$(CH)chmod 755 $@
	$(CH)chgrp sys $@
	$(CH)chown root $@

clean:
	for cmd in $(COMMANDS) ; \
	do \
		(cd $$cmd; $(MAKE) -f $$cmd.mk clean); \
	done

clobber:	clean
