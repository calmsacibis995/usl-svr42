#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/fdscripts/mv2swap.sh	1.13"

# This script reserves part of the swap slice for a temporary
# root file system, which will hold the contents of the second
# floppy, so that we can unmount the second floppy and mount
# the third one. The vtoc is changed (in core only, not on
# the hard disk, so that slice 5 points to this area.

#main()
. ${SCRIPTS}/common.sh

cd /
EXITSTATUS=""

# fakevtoc divvies up the swap slice.
fakevtoc /dev/rdsk/c0t0d0s0
EXITSTATUS="$?"
[ "${EXITSTATUS}" != "0" ] && {
	[ "${UPDEBUG}" = "Yes" ] && goany
	error_out tmproot
}
sync
sync
sync
sync

# add portion of swap not used for temporary file system to
# swap space -- we'll need it to mkfs
addswap -s /dev/rdsk/c0t0d0s0 /dev/dsk/c0t0d0s2 2

EXITSTATUS="$?"
[ "${EXITSTATUS}" != "0" ] && {
	[ "${UPDEBUG}" = "Yes" ] && goany
	error_out tmproot
}

EXITSTATUS="1"
while [ "${EXITSTATUS}" != "0" ]
do
	# how much space do we have in temporary root fs to create
	# a file system?
	rootsize=`slicesize -s /dev/rdsk/c0t0d0s0 5`
	/etc/fs/s5/mkfs -b 512 /dev/rdsk/c0t0d0s5 $rootsize:1000 > /dev/null 2>&1
	EXITSTATUS="$?"
	if [ "${EXITSTATUS}" != "0" ]
	then
		[ "${UPDEBUG}" = "Yes" ] && goany
		error_out tmproot
	else
		instcmd mount /dev/dsk/c0t0d0s5 /mnt
		EXITSTATUS="$?"
		[ "${EXITSTATUS}" != "0" ] && {
			[ "${UPDEBUG}" = "Yes" ] && goany
			error_out tmproot
		}
	fi
done

menu_colors regular
menu -r -f ${FD_MENUS}/copyfiles.1 -o /tmp/response 2>/dev/null
cd ${MOUNTED_ROOT}
find . -print | cpio -pdum /mnt > /dev/null 2>&1
sync
sleep 1
sync
sleep 1
[ ! -d /mnt/etc/inst/scripts ] && {
	[ "${UPDEBUG}" = "Yes" ] && goany
	error_out tmproot
}
[ ! -f /mnt/etc/inst/scripts/getFloppy3.sh ] && {
	[ "${UPDEBUG}" = "Yes" ] && goany
	error_out tmproot
}

# it will be remounted as /MOUNTED_ROOT once we unmount the 2nd floppy
instcmd umount  /mnt 
EXITSTATUS="$?"
[ "${EXITSTATUS}" != "0" ] && {
	[ "${UPDEBUG}" = "Yes" ] && goany
	error_out tmproot
}
sync
sync
sync
