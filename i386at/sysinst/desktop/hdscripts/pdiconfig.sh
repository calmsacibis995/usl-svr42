#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/pdiconfig.sh	1.2"

# the PDI tools will create SCSI disk devices with names to match
# the physical names of the devices. The names used on the boot
# floppies may be wrong. For the tapes, they certainly are.
# We use /dev/rmt/tape* on the boot flops, pdi uses /dev/rmt/qtape*

# this script runs pdiconfig to turn off unnecessary PDI modules
# and drivers. It then runs pdimkdev to recreate the disk nodes
# with the right names. Finally, pdimkdtab is run to build a
# device.tab with the PDI names for the tape devices in it.

# Because the names of the disks may change, we need to go back
# and edit vfstab to reference the correct disk name.

# Interrupted install makes things a little more tricky. 
# We need to be able to recover "boot floppy names" for the
# devices so that if the user DEL's out, gets into the shell,
# and exits "1" to restart, that we can recover.


#take input in form of ls -l output for /dev/dsk/c?t?d?s? and cut
#to c?t?d? output
get_disk_name()
{
	shift 	#make arg 10 positionally arg 9 for non-ambiguous $ reference
	NAME=$9
	NAME=`expr substr ${NAME} 10 6`
	echo $NAME
}


# main()
. ${SCRIPTS}/common.sh

# pop a screen up while we're doing all this....
# it will last through to the end of rebuild.sh (the kernel has been
# rebuilt.

menu_colors regular
menu -r -f ${HD_MENUS}/rebuild.1 -o /tmp/response 2>/dev/null

# echo "Before rebuilding. Have a Shell"
# /sbin/sh
> /tmp/Errors

# remove /etc/scsi/pdi_edt just in case one is already there --
# force a re-initialization
rm -rf /etc/scsi/pdi_edt 1>/dev/null 2>&1

# save existing device files in /dev/.o<> directories
# only need to save c* files. The 0s* and 1s* special files
# are OK as is. For tapes, just move everything out of /dev/rmt.

mkdir -p /dev/.ordsk 1>/dev/null 2>&1
cd /dev/rdsk
[ -r c* ] && mv c* ../.ordsk 1>/dev/null 2>&1 # only copy if files to copy

mkdir -p /dev/.odsk 1>/dev/null 2>&1
cd /dev/dsk
[ -r c* ] && mv c* ../.odsk 1>/dev/null 2>&1

mkdir -p /dev/.ormt 1>/dev/null 2>&1
cd /dev/rmt
[ -r * ] && mv * ../.ormt 1>/dev/null 2>&1

cd /

# stdout not redirected because there may be a question from pdimkdev
# re: running out of inodes -- it will ask you if this is the boot
# disk.

/etc/scsi/pdimkdev -fs 2>&1

# The following updates /etc/vfstab so that the correct
# names for the first and second disk are used rather than
# the names c0t0d0* and c0t1d0* that are hard-coded on the boot
# floppies. Save old vfstab in /etc/inst/.ovfstab

cp /etc/vfstab /etc/inst/.ovfstab 1>/dev/null 2>&1

FIRSTDISK=`ls -l /dev/dsk/c?t?d?s0 | grep ",  0 "`
[ -n "${FIRSTDISK}" ] && {
   FIRSTDISK=`get_disk_name $FIRSTDISK`
   cat /etc/vfstab | sed s/c0t0d0/${FIRSTDISK}/g > /tmp/vfstab
   mv /tmp/vfstab /etc
}
SECDISK=`ls -l /dev/dsk/c?t?d?s0 | grep ", 16 "`
[ -n "${SECDISK}" ] && {
   SECDISK=`get_disk_name $SECDISK`
   cat /etc/vfstab | sed s/c0t1d0/${SECDISK}/g > /tmp/vfstab
   mv /tmp/vfstab /etc
}

# run pdiconfig to turn off all possible SCSI drivers....

/etc/scsi/pdiconfig /etc/scsi/.pdicfglog > /dev/null 2>&1 && /etc/scsi/diskcfg /etc/scsi/.pdicfglog > /dev/null 2>&1
rm -f /etc/scsi/.pdicfglog 1>/dev/null 2>&1

#rebuild device.tab. Save old one as /etc/inst/.odevice.tab

cp /etc/device.tab /etc/inst/.odevice.tab 1>/dev/null 2>&1
grep -v diskette /etc/device.tab | grep -v "^Ntape" | grep -v "^ctape" > /tmp/device.tab 2>&1
mv /tmp/device.tab /etc/device.tab
/usr/sadm/sysadm/bin/mkdtab 1>/dev/null 2>&1
/etc/scsi/pdimkdtab -fis 1>/dev/null 2>&1

rm -rf /etc/scsi/pdi_edt 1>/dev/null 2>&1

# Next, rebuild kernel (next script).
