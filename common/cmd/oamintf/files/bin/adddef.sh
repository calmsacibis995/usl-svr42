#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/adddef.sh	1.2.5.3"
#ident  "$Header: adddef.sh 2.1 91/09/12 $"

DEV=$1
RDEV="-"
FS=$2
AUTOMNT=$3
FSTYPE=$4
MNTOPT1=$5
MNTOPT2=$6
LEVEL=$7
if test -b "$DEV"
then
	BDEVICE="$DEV"
else
	BDEVICE=`/usr/bin/devattr "$DEV"  bdevice 2>/dev/null`
	if test "$BDEVICE" != ""
	then
		DEV="$BDEVICE"
	fi
fi
RDEVICE=`/usr/bin/devattr "$DEV"  cdevice 2>/dev/null`
if test "$RDEVICE" != ""
then
	RDEV="$RDEVICE"
fi
if test "$MNTOPT1" = "read-only"
then
	MNTOPT1="ro"
else
	MNTOPT1="rw"
fi
if test "$MNTOPT2" = "yes"
then
	MNTOPTS="$MNTOPT1,suid"
else
	MNTOPTS="$MNTOPT1,nosuid"
fi

# printf command cannot override DAC and MAC restrictions
# set on /etc/vfstab file, so we'll need to use /usr/bin/cp
if [ $TFADMIN ]
then
	/usr/bin/cp /etc/vfstab /tmp/vfstab
	/usr/bin/printf '%-17s %-17s %-6s %-6s %-8s %-7s %-8s %-20s\n' $DEV $RDEV $FS $FSTYPE "-" $AUTOMNT $MNTOPTS "$LEVEL" >>/tmp/vfstab
	$TFADMIN /usr/bin/cp /tmp/vfstab /etc/vfstab
else
	/usr/bin/printf '%-17s %-17s %-6s %-6s %-8s %-7s %-8s %-20s\n' $DEV $RDEV $FS $FSTYPE "-" $AUTOMNT $MNTOPTS "$LEVEL" >>/etc/vfstab
fi
