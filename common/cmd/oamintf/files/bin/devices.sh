#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/devices.sh	1.1.6.2"
#ident  "$Header: devices.sh 2.1 91/09/12 $"

FSYS=$1

#We dominate /etc/vfstab's level
#No privileges need to read from it.

while read bdev cdev mountp dummy
do
	case $bdev in
	'#'*| ' ')
		continue;;
	esac
	if [ "$mountp" =  "$FSYS" ]
	then
		if [ -b $bdev -o -c $bdev ]
		then
			echo $bdev
		fi
	fi
done < /etc/vfstab
