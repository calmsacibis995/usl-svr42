#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)face:face.mk	1.3.5.4"
#ident "$Header: face.mk 1.4 91/06/27 $"

#
# The BIG makefile for ViewMaster - should be run as root
#

include $(CMDRULES)

all install clean clobber:
	@set -e;\
	cd src;\
	$(MAKE) -f src.mk $(MAKEARGS) $@;\
	echo FINISHED

