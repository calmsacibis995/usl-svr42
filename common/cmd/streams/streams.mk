#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)cmd-streams:streams.mk	1.3.5.2"
#ident "$Header: streams.mk 1.2 91/03/20 $"

include $(CMDRULES)

all install:
	cd log ;    $(MAKE) -f str.mk $(MAKEARGS) $@
	cd strcmd ; $(MAKE) -f strcmd.mk $(MAKEARGS) $@
	cd kmacct ; $(MAKE) -f kmacct.mk $(MAKEARGS) $@

clean clobber lintit:
	cd log ;    $(MAKE) -f str.mk $(MAKEARGS) $@
	cd strcmd ; $(MAKE) -f strcmd.mk $(MAKEARGS) $@
	cd kmacct ; $(MAKE) -f kmacct.mk $(MAKEARGS) $@
