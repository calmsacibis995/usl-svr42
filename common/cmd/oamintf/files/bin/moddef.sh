#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/moddef.sh	1.1.9.4"
#ident  "$Header: moddef.sh 2.1 91/09/12 $"

NEWDEV=$1
NEWMOUNTP=$2
FSTYPE=$3
AUTOMNT=$4
RW=$5
SUID=$6
BDEV=$7
MOUNTP=$8
LEVEL=$9
> /tmp/vfstab
RDEV=`/usr/bin/devattr "$NEWDEV" cdevice 2> /dev/null`
if [ $? -ne 0 ]
then
	RDEV=-
fi
if [ ! -b $BDEV ]
then
	BDEV=`/usr/bin/devattr "$BDEV" bdevice 2>/dev/null`
fi
if [ ! -b $NEWDEV ]
then
	NEWDEV=`/usr/bin/devattr "$NEWDEV" bdevice 2>/dev/null`
fi
MNTOPTS="-"
if [ "$SUID" = "yes" ]
then
	MNTOPTS="suid"
else
	MNTOPTS="nosuid"
fi
if [ "$RW" = "read-only" ]
then
	if [ "t$MNTOPTS" = "t-" ]
	then
		MNTOPTS="ro"
	else
		MNTOPTS="$MNTOPTS,ro"
	fi
fi
while read bdev rdev mountp fstype fsckpass automnt mntopts level
do
	if test "$BDEV" = "$bdev" -a "$MOUNTP" = "$mountp"
	then
		/usr/bin/printf '%-17s %-17s %-6s %-6s %-8s %-7s %-8s\n %-20s' ${NEWDEV} ${RDEV} ${NEWMOUNTP} ${FSTYPE} "-" ${AUTOMNT} ${MNTOPTS} "${LEVEL}" >> /tmp/vfstab
		continue
	fi
	/usr/bin/printf '%-17s %-17s %-6s %-6s %-8s %-7s %-8s %-20s\n' ${bdev} ${rdev} ${mountp} ${fstype} ${fsckpass} ${automnt} ${mntopts} "${level}">> /tmp/vfstab
done < /etc/vfstab

$TFADMIN cp /tmp/vfstab /etc/vfstab
