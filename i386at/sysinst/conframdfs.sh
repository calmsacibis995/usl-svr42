#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:conframdfs.sh	1.2.1.5"

NM="${PFX}nm"
DUMP="${PFX}dump"
STRIP="${PFX}strip"
UNIXSYMS="${PFX}unixsyms"
MCS="${PFX}mcs"

BASE=${ROOT}/${MACH}/
EDSYM="desktop/instcmd/edsym"

KERNEL=${BASE}/stand/unix
RAMPROTO="desktop/ramdfs.proto"

sizeline=""
sizeline=`${NM} -px ${KERNEL} | grep RootRamDiskBuffer`
if [ "$sizeline" = "" ]
then
	echo "Cannot find symbol RootRamDiskBuffer in file ${KERNEL}"
	exit 1
fi
set $sizeline
#echo  $1 $2 $3

if [ "$3" != "RootRamDiskBuffer" ]
then
	echo "Cannot find symbol RootRamDiskBuffer in file ${KERNEL}"
	exit 1
fi
sizeaddr=$1
bufferaddr=$1
#echo address of RootRamDiskBuffer is $1
dsection=""
doffset=""

ROOTFS=/tmp/ramdisk.fs

tmpfile=/tmp/tmp$$
grep -v "^#" ${RAMPROTO} > ${tmpfile}
sed "s,\$ROOT\/\$MACH,$ROOT/$MACH,p" ${tmpfile} > /tmp/ramdproto

> ${ROOTFS}
/sbin/mkfs -Fs5 -b 512 ${ROOTFS} /tmp/ramdproto 2 36
rm ${tmpfile} /tmp/ramdproto
sync

${DUMP} -hv ${KERNEL} | grep data | while read x1 x2 x3 section offset x4 name 
do
	[ "$name" = ".data" ] && {
		${EDSYM} -f ${ROOTFS} $section $offset $bufferaddr ${KERNEL}
		exit 55
	}
done
if [ "$?" != "55" ]
then
	echo "Cannot find a .data setion ${KERNEL}"
	exit 1
fi
