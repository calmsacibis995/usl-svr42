#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/fstype.sh	1.1.4.1"
#ident	"$Header: $"
# fstype <mountp> <special>
# returns the type of special from vfstab (if any)
MOUNTP=$1
SPECIAL=$2
while read special dummy1 mountp fstype dummy2
do
	if [ "t$SPECIAL" = "t$special" -a "t$MOUNTP" = "t$mountp" ]
	then
		echo $fstype
		exit 0
	fi
done < /etc/vfstab
