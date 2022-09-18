#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/fsnames.sh	1.1.6.2"
#ident  "$Header: fsnames.sh 2.1 91/09/12 $"

DEV=$1
if test -b $DEV
then
	BDEVICE=$DEV
else
	BDEVICE=`/usr/bin/devattr "$DEV" bdevice 2>/dev/null`
	if test "$BDEVICE" = ""
	then
		BDEVICE=$DEV
	fi
fi
# We dominate vfstab file, no privs needed to read it
while read device dummy mountp dummy2
do
	if test "$BDEVICE" = "$device"
	then
		echo "$mountp"
	fi
done < /etc/vfstab

