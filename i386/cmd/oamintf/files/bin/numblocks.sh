#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:i386/cmd/oamintf/files/bin/numblocks.sh	1.1"
#ident	"$Header: $"

DEVICE=$1
#get the block device
if [ ! -b $DEVICE ]
then
	BDEVICE=`/usr/bin/devattr "$DEVICE" bdevice  2>/dev/null`
else
	BDEVICE="$DEVICE"
fi
# if device is diskette use defaults else read size from prtvtoc o/p.
DEVALIAS=`/usr/bin/devattr $DEVICE alias | /usr/bin/sed 's/^\(.*\).$/\1/'`
if [ "$DEVALIAS" = "diskette" ]
then
	#blocksize=1422
	blocksize=2048
else
	blocksize=`/usr/bin/devattr $BDEVICE capacity`

fi
echo "$blocksize"
exit 0
