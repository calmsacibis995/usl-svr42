#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/filsys.sh	1.2.3.2"
#ident  "$Header: filsys.sh 2.1 91/09/12 $"

## We dominate vfstab file, no privs needed to read it
while read bdev cdev mountp dummy
do
	case $bdev in
	'#'*| ' ')
		continue;;
	esac
	if [ "$mountp" !=  "/usr" -a "$mountp" != "/" ]
	then
		echo $mountp
	fi
done < /etc/vfstab
