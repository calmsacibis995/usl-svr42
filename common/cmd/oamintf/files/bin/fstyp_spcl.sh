#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/fstyp_spcl.sh	1.2.3.3"
#ident  "$Header: fstyp_spcl.sh 2.1 91/09/12 $"

# fstyp_spcl  <special>
# returns the type of special from vfstab (if any)
SPECIAL=$1
BLOCK=`/usr/bin/devattr "$SPECIAL" bdevice  2> /dev/null`
if [ "$BLOCK" = "" ]
then
	BLOCK="$SPECIAL"
fi
# We dominate vfstab file, no privs needed to read it
while read special dummy1 mountp fstype dummy2
do
	if [ "t$BLOCK" = "t$special" ]
	then
		echo $fstype
		exit 0
	fi
done < /etc/vfstab
