#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/indeftab.sh	1.3.3.2"
#ident  "$Header: indeftab.sh 2.1 91/09/12 $"

DEV=$1
FS=$2
if test -b $DEV
then
	BDEVICE="$DEV"
else
	BDEVICE=`/usr/bin/devattr "$DEV"  bdevice 2>/dev/null`
	if test "$BDEVICE" = ""
	then
		BDEVICE="$DEV"
	fi
fi
# We dominate vfstab file, no privs needed to read it
while read bdev rdev mountp fstype fsckpass automnt mountopts
do
	case $bdev in
	'#'* | '' )
		continue;;
	'-')
		continue
	esac
	if test "$BDEVICE" = "$bdev" -a "$FS" = "$mountp"
	then
		echo "$mountopts" > /tmp/mntopts
		/usr/bin/grep "-" /tmp/mntopts  > /dev/null
		if [ $? -eq 0 ]
		then 
			mntopts="read/write"
			setuid="no"
		fi
		/usr/bin/grep "rw" /tmp/mntopts  > /dev/null
		if [ $? -eq 0 ]
		then 
			mntopts="read/write"
			setuid="no"
		fi
		/usr/bin/grep "ro" /tmp/mntopts  > /dev/null
		if [ $? -eq 0 ]
		then 
			mntopts="read-only"
		fi
		setuid="yes"	
		/usr/bin/grep  "nosuid" /tmp/mntopts > /dev/null
		if [ $? -eq 0 ]
		then 
			setuid="no"
		fi
		if [ "t" = "t$mntopts" ]
		then
			mntopts="read/write"
		fi
		echo $fstype $mntopts $setuid $automnt > /tmp/indeftab
		echo "true"
		exit 0
	else
		continue
	fi
done < /etc/vfstab
exit 1
