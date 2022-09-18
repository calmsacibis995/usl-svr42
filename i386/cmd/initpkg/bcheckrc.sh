#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)initpkg:i386/cmd/initpkg/bcheckrc.sh	1.5.30.13"
#ident "$Header: $"


rootfs=/dev/root
vfstab=/etc/vfstab


test -x /sbin/dumpcheck && /sbin/dumpcheck
echo MARK | /sbin/dd of=/dev/rswap conv=sync >/dev/null 2>&1


# Complete MAC initialization before mounting anything else.
test -x /sbin/macinit && /sbin/macinit

# Set the console state to public.
/sbin/consalloc 2

# put root into mount table
echo "${rootfs} /" | /sbin/setmnt


# Initialize name of system from name stored in /etc/nodename.
[ -s /etc/nodename ] && read node </etc/nodename && [ -n "$node" ] &&
	/sbin/uname -S "$node"
set `/sbin/uname -a`

/sbin/mount /proc > /dev/null 2>&1

/sbin/mount /dev/fd > /dev/null 2>&1

while read bdevice rdevice mntp fstype fsckpass automnt mntopts
do
	# check for comments
	case ${bdevice} in
	'#'*)	continue
	esac

	# see if this is /stand or /var - check and mount if it is
	if [ "${mntp}" = "/stand" -o "${mntp}" = "/var" ]
	then
		/sbin/fsck -F ${fstype} -m  ${rdevice}  >/dev/null 2>&1
		if [ $? -ne 0 ]
		then
			if [ ! -f /etc/.fscklog ]
			then
				echo > /etc/.fscklog
				echo "Please wait while the system is examined.\
  This will take a few minutes.\n"
			fi
			/sbin/fsck -F ${fstype} -y  ${rdevice} >/dev/null 2>&1
		fi
		/sbin/mount ${mntp} > /dev/null 2>/dev/null
	fi
done < $vfstab

if [ -x /sbin/fdinit ]
then
	rm -f /dev/fd0 /dev/fd1 /dev/rfd0 /dev/rfd1
	type=`/sbin/fdinit -f 0`
	if [ "$type" -eq "3" ]
	then
		/sbin/ln /dev/dsk/f03ht /dev/fd0
		/sbin/ln /dev/rdsk/f03ht /dev/rfd0
	elif [ "$type" -eq "5" ]
	then
		/sbin/ln /dev/dsk/f05ht /dev/fd0
		/sbin/ln /dev/rdsk/f05ht /dev/rfd0
	fi

	type=`/sbin/fdinit -f 1`
	if [ "$type" -eq "3" ]
	then
		/sbin/ln /dev/dsk/f13ht /dev/fd1
		/sbin/ln /dev/rdsk/f13ht /dev/rfd1
	elif [ "$type" -eq "5" ]
	then
		/sbin/ln /dev/dsk/f15ht /dev/fd1
		/sbin/ln /dev/rdsk/f15ht /dev/rfd1
	fi

fi
