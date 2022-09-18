#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:cmd/readfloppy.sh	1.3.3.1"
#ident	"$Header: $"

LABEL_LOC=$1

echo "\nInstallation is in progress -- do not remove the floppy disk."
retry=`expr 0`
seq=`dd if=/dev/rdsk/f0t count=1 bs=512 skip=${LABEL_LOC:=29} 2>/dev/null`
while [ $? -ne 0 -a $retry -le 2 ]
do
	retry=`expr $retry + 1`
	seq=`dd if=/dev/rdsk/f0t count=1 bs=512 skip=${LABEL_LOC} 2>/dev/null`
done
ERR=$?
if [ $ERR = 0 ]
then echo "$seq" > /tmp/seq
fi
exit $ERR
