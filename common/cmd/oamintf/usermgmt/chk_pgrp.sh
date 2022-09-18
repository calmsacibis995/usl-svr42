#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/chk_pgrp.sh	1.2.7.2"
#ident  "$Header: chk_pgrp.sh 2.0 91/07/12 $"

################################################################################
#	Command Name: chk_pgrp
#
# 	This functions is used for validating the Primary Group
################################################################################

NOTHING=1	# Nothing entered
NOTUNIQ=2	# Not unique
OK=0		# login name (ID) is valid

test -z "$1" && exit $NOTHING

DEF_GRPNAM="other"
DEF_GRPID="1"

test "$1" = "$DEF_GRPNAM" || test "$1" = "$DEF_GRPID" && exit $OK

/usr/bin/sed -e 's/^\([^:]*\):[^:]*:\([^:]*\).*/\1,\2/p' \
-e '/,[0-9]\{1,2\}$/d' /etc/group | /usr/bin/grep "^$1," > /dev/null ||

/usr/bin/sed -e 's/^\([^:]*\):[^:]*:\([^:]*\).*/\1,\2/p' \
-e '/,[0-9]\{1,2\}$/d' /etc/group | /usr/bin/grep ",$1$" > /dev/null && exit $OK || exit 1
