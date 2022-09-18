#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/uniq_gid.sh	1.2.8.2"
#ident  "$Header: uniq_gid.sh 2.0 91/07/12 $"

################################################################################
#	Command Name: uniq_gid
#
# 	This functions is used for checking 3 things: 1) input was entered
#	2) if default is input 3) if group ID is unique.
#
#	$1 - User input
#	$2 - Field default (only in groupmod)
################################################################################

# Field is mandatory
test -z "$1" && exit 1

# Is field same as default?
test "$1" = "$2" && exit 0

# Is gid unique
if /usr/bin/grep ":$1:" /etc/group > /dev/null 2>&1
then
	exit 2
else
	exit 0
fi
