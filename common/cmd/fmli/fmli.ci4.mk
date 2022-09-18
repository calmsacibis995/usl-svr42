#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#
# Copyright  (c) 1985 AT&T
#	All Rights Reserved
#
#ident	"@(#)fmli:fmli.ci4.mk	1.2.2.4"
#ident "$Header: fmli.ci4.mk 1.3 91/08/13 $"
#

include $(CMDRULES)

CURSES_H=$(INC)

DIRS =	form menu oeu oh proc qued sys vt wish gen xx 

.DEFAULT:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		/bin/echo Making $@ in $$d subsystem;\
		$(MAKE) -f $$d.mk $(MAKEARGS) CURSES_H="$(CURSES_H)" GLOBALDEF="$(GLOBALDEF) -DPRE_CI5_COMPILE -DNO_MOUSE" EXTRA_LIBS="-lPW" $@;\
		cd ..;\
	done;\
	/bin/echo 'fmli.mk: finished making target "$@"'

