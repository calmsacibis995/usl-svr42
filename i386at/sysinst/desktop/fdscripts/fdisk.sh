#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/fdscripts/fdisk.sh	1.9"

# This shell script runs the fdisk command to create at least a
# ${MIN_HARDDISK} Mb partition on the first hard disk, and if the user
# selects to do so, at least a ${MIN_SECDISK} Mb partition on the
# second hard disk.

#
# Function definitions
#

Partition_The_Disks () {

 #custom fdisk for 1st disk

 MENU_TYPE=regular
 SIZE_WARN=No
 while [ 1 ]
 do
	menu_colors ${MENU_TYPE}
 	menu -f ${FD_MENUS}/part.1 -o /tmp/resp.$$ 2>/dev/null
	fdisk -I /dev/rdsk/c0t0d0s0
	rc=$?
	DISK_SIZE=`partsize /dev/rdsk/c0t0d0s0`
	rc1=$?
	# if either fdisk or partsize failed, something is very wrong
	[ "${rc}" != 0 -o "${rc1}" != "0" ] && error_out unexpdisk

	if [ "${DISK_SIZE}" -ge "${MIN_HARDDISK}" ]
	then
		break;
	fi
	SIZE_WARN=Yes
	MENU_TYPE=warn
 done
 unset MENU_TYPE

 # There is no need for V_REMOUNTing the drive  because the driver will pick
 # the new  partition table anyway.

 # check to see if theres a second disk. Assume No and set to yes only
 # explicitly
 SECOND_DISK=No
 SECOND_SIZE=0
 SECOND_SIZE=`partsize -s /dev/rdsk/c0t1d0s0  2>/dev/null`
 [ "${SECOND_SIZE}" -lt "${MIN_SECDISK}" ] && {
	   return
 }
 SIZE_WARN=No
 MENU_TYPE=regular
 while [ 1 ]
 do
	unset RETURN_VALUE
	menu_colors ${MENU_TYPE}
        menu -f ${FD_MENUS}/part.2 -o /tmp/resp.$$ 2>/dev/null
        . /tmp/resp.$$
	rm -rf /tmp/resp.$$ 1>/dev/null 2>/dev/null
        [ `expr ${RETURN_VALUE}` = 1 ] && {
	  SECOND_DISK=No
	  return
        }

        #custom_fdisk /dev/rdsk/c0t1d0s0 for disk2
        fdisk -I /dev/rdsk/c0t1d0s0
	rc=$?
	DISK_SIZE=0
	# if fdisk on second hard disk fails, assume size of 0
	# otherwise, get size of unix partition via partsize
	[ "${rc}" = 0 ] && {
           DISK_SIZE=`partsize /dev/rdsk/c0t1d0s0`
	}
		
        if [ "${DISK_SIZE}" -ge "${MIN_SECDISK}" ]
        then
	   SECOND_DISK=Yes
   	   return ;
        fi
	SIZE_WARN=Yes
	MENU_TYPE=warn
 done
 unset MENU_TYPE
}

# main()

cd /
. ${SCRIPTS}/common.sh

rm -f /core ${LOCKFILE} 1>dev/null 2>/dev/null

Partition_The_Disks

echo "\nSECOND_DISK=\"$SECOND_DISK\"" >> $GLOBALS
