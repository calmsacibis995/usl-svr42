#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/validdev.sh	1.1.6.2"
#ident  "$Header: validdev.sh 2.1 91/09/12 $"

DEV=$1
ALIAS=
if /usr/bin/getdev $1 > /tmp/alias001
then
	read ALIAS < /tmp/alias001
	if test  "t$DEV" = "t$ALIAS" 
	then
		echo "$DEV is a valid device or alias"
		/usr/bin/rm -f  /tmp/alias001
		exit 0
	else
		if test -c "$DEV" -o -b "$DEV" 
		then
			echo "$DEV is a valid device"
			/usr/bin/rm -f  /tmp/alias001
			exit 0
		else
			echo "$DEV not block or character device or alias"
			/usr/bin/rm -f  /tmp/alias001
			exit 1
		fi
	fi
fi
/usr/bin/rm -f  /tmp/alias001
exit 1
