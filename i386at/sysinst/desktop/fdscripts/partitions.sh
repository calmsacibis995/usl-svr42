#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/fdscripts/partitions.sh	1.2.1.15"
#	Portions Copyright (C) 1990, 1991 Intel Corporation.
# 	Portions Copyright (C) 1990 Interactive Systems Corporation.
# 	All Rights Reserved

#
# This script sets up the file /tmp/hdscripts.sh and /tmp/hd.0.lay
# (and optionally /tmp/hd.1.lay). The hdscripts.sh file contains
# calls to disksetup passing the hd.*.lay files as templates for
# creating file systems on the hard disk. 
#
# The values that are put into these files are those for Automatic
# Installation. If the user selected custom installation, the
# script custom_fs.sh will create replacements for these files.

#
# Function definitions
#

percentage()
{
	percent=`expr $2 \* $1 `
	percent=`expr ${percent} + 99` # round up
	percent=`expr ${percent} / 100 `
	echo $percent
}

# main()

cd /
set +a
MEMSIZE="`memsize`"
. ${SCRIPTS}/common.sh


[ $INSTALL_TYPE = NEWINSTALL ] || { 	
	# skip to setinsttyp.sh to  attempt upgrade/overlay install
	exit 0
}
rm -f /core ${LOCKFILE}

# get out of this script if custom installation was selected --
# data will be supplied by the user
[ $INSTALL_MODE = CUSTOM ] && {
	exit 0
}

# create ROOTFSTYPE file, stripping out gunk and blank lines
# so read < ROOTFSTYPE works.
grep -v "^#" /etc/autofstype | grep -v "^$" > ${ROOTFSTYPE}
grep -v "^#" /etc/autodsk0.lay | grep -v "^$" > /tmp/hd.0.lay

tprimsiz=`partsize /dev/rdsk/c0t0d0s0`

# we define STANDSIZE here because we need the size to reserve for
# /stand. A better approach would be incorporating it into a file,
# but that will be left for a later release.

STANDSIZE=5 	# Megabytes

[ "${tprimsiz}" -gt 150 ] && STANDSIZE=10 # reset STANDSIZE if disk big

primsiz=`eval expr $tprimsiz`
primsiz=`expr ${primsiz} - 1` # Reserve last cylinder for DOS
secsiz="0"
[ ${SECOND_DISK} = Yes ] && {
	grep -v "^#" /etc/autodsk1.lay | grep -v "^$" > /tmp/hd.1.lay
	tsecsiz=`partsize /dev/rdsk/c0t1d0s0`
	eval secsiz=${tsecsiz}
	secsiz=`expr ${secsiz} - 1` # Reserve last cylinder for DOS
}

# compute swap size -- minimum of (a) or (b)
# (a) round memsize up, multiply by two.
swapsize=`expr ${MEMSIZE} + 1048575`
swapsize=`expr ${swapsize} / 1048576`
swapsize=`expr ${swapsize} \* 2`

# (b) is 13% partition size; set swap size to minimum of (a) or (b)
# if partition size is less than RECMIN_HARDDISK (recommended hard disk
# size) then use RECMIN_HARDDISK as the partition size.
PARTSIZE=${tprimsiz}
[ ${PARTSIZE} -lt ${RECMIN_HARDDISK} ] && {
	PARTSIZE=${RECMIN_HARDDISK}
}
altswapsize=`percentage ${PARTSIZE} 13`
[ "${altswapsize}" -lt "${swapsize}" ] && {
	swapsize=${altswapsize}
}

# save 2 percent of disk for bad block alternates table
# Enhancement -- if we can find way to only do this for non-SCSI

altreserv=`percentage ${primsiz} 2`

# allocate all space except for swap, stand and alternates to root
pleft="`expr ${primsiz} - ${swapsize}`"
pleft="`expr ${pleft} - ${STANDSIZE}`"
pleft="`expr ${pleft} - ${altreserv}`"

# now do second hard disk. Save 2 percent for alts
altsreserv=`percentage ${secsiz} 2`
secsiz="`expr ${secsiz} - ${altsreserv}`"

if [ "${secsiz}" != "0" ]
then
	# substitute the size of the second disk partition
	# minus space for alternates in parts file for disk 1
	ed /tmp/hd.1.lay 1>/dev/null 2>/dev/null <<!EOF!
/HOME
s/HOME/${secsiz}/g
w
w
q
!EOF!
fi

# substitute the size of the first disk partition
# minus space for alternates in parts file for disk 0
ed /tmp/hd.0.lay 1>/dev/null 2>/dev/null <<!EOF!
/SWAP
s/SWAP/${swapsize}/g
/ROOT
s/ROOT/${pleft}/g
/STAND
s/STAND/${STANDSIZE}/g
w
w
q
!EOF!
echo ". /etc/inst/scripts/common.sh" > /tmp/hdscripts.sh
echo "menu_colors regular" >> /tmp/hdscripts.sh
echo "menu -r -f ${FD_MENUS}/part.9 -o /dev/null 2>/dev/null" >> /tmp/hdscripts.sh
echo "disksetup -m ${MEMSIZE} -x /tmp/disksetup.sh -d /tmp/hd.0.lay -IsB -b /etc/boot /dev/rdsk/c0t0d0s0" >> /tmp/hdscripts.sh
echo "[ \"\$?\" != \"0\" ] && error_out unexpdisk" >> /tmp/hdscripts.sh
[ ${SECOND_DISK} = Yes ] && {
	echo disksetup -m ${MEMSIZE} -x /tmp/disksetup.sh -d /tmp/hd.1.lay -Is /dev/rdsk/c0t1d0s0 >> /tmp/hdscripts.sh
	echo "[ \"\$?\" != \"0\" ] && error_out unexpdisk2" >> /tmp/hdscripts.sh
	}
