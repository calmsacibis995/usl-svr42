#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/dispdef.sh	1.3.5.3"
#ident  "$Header: dispdef.sh 2.1 91/09/12 $"

#set -x
DEV=$1
FS=$2
ES=$3
entry="false"
/usr/bin/rm /tmp/vfstabdisp 2>/dev/null
if [ "$ES" = "yes" ]; then
echo "Mount Device      Filesystem   Automount  Type  Mount Options Level\n------------      ----------   ---------  ----  -------------
-----\n" >> /tmp/vfstabdisp
else
echo "Mount Device      Filesystem   Automount  Type  Mount Options\n------------      ----------   ---------  ----  -------------\n" >> /tmp/vfstabdisp
fi

if [ "$DEV" = "ALL" -a "$FS" = "ALL" ]
then
	## We dominate vfstab level, no privs needed to read from it ##

	while read bdev rdev mountp fstype fsckpass automnt mntopts level
	do
		case $bdev in
		'#'* | '' )
			continue;;
		'-')
			continue
		esac
		if [ "$ES" = "yes" ]; then	
			/usr/bin/printf '%-18s%-12s%7s%8s%15s%13s\n' ${bdev} ${mountp} ${automnt} ${fstype} ${mntopts} ${level} >> /tmp/vfstabdisp
		else
			/usr/bin/printf '%-18s%-12s%7s%8s%15s\n' ${bdev} ${mountp} ${automnt} ${fstype} ${mntopts} >> /tmp/vfstabdisp
		fi

	done < /etc/vfstab
	exit 0
fi
if [ "$DEV" = "ALL" ]
then
	## We dominated vfstab, no privs needed to read from it ##
	/usr/bin/grep "$FS" /etc/vfstab >/tmp/fsdisp
	while read bdev rdev mountp fstype fsckpass automnt mntflags level
	do
		if [ "$ES" = "yes" ]; then
                	/usr/bin/printf '%-18s%-12s%7s%8s%15s%13s\n' $bdev $mountp $automnt $fstype $mntflags $level >>/tmp/vfstabdisp
		else	
                /usr/bin/printf '%-18s%-12s%7s%8s%15s\n' $bdev $mountp $automnt $fstype $mntflags >>/tmp/vfstabdisp
		fi	
	done < /tmp/fsdisp
	exit 0
fi
if [ "$FS" = "ALL" ]
then
	## We dominated vfstab, no privs needed to read from it ##
	/usr/bin/grep "$DEV" /etc/vfstab >/tmp/devdisp
	while read bdev rdev mountp fstype fsckpass automnt mntflags level
	do
		if [ "$ES" = "yes" ]; then
                	/usr/bin/printf '%-18s%-12s%7s%8s%15s%13s\n' $bdev $mountp $automnt $fstype $mntflags $level >>/tmp/vfstabdisp
		else	
                /usr/bin/printf '%-18s%-12s%7s%8s%15s\n' $bdev $mountp $automnt $fstype $mntflags >>/tmp/vfstabdisp
		fi	
	done < /tmp/devdisp
	exit 0
fi

BDEVICE=`/usr/bin/devattr "$DEV"  bdevice 2>/dev/null`
if test "$BDEVICE" != ""
then
	DEV="$BDEVICE"
fi

# We dominate vfstab, no privs needed to read from it
exec < /etc/vfstab
while read bdev rdev mountp fstype fsckpass automnt mntopts
do
	case $bdev in
	'#'* | '' )
		continue;;
	'-')
		continue;;
	esac
	if test "$DEV" != "$bdev" -o "$FS" != "$mountp"
	then
		continue
	fi
	entry="true"
	if [ "$ES" = "yes" ]; then	
		/usr/bin/printf '%-18s%-12s%7s%8s%15s%13s\n' ${bdev} ${mountp} ${automnt} ${fstype} ${mntopts} ${level} >> /tmp/vfstabdisp
	else
		/usr/bin/printf '%-18s%-12s%7s%8s%15s\n' ${bdev} ${mountp} ${automnt} ${fstype} ${mntopts} >> /tmp/vfstabdisp
	fi
	exit 0
done
if [ "$entry" = "false" ]
then
 	echo "Defaults for this file system do not exist." > /tmp/vfstabdisp
fi
exit 0

