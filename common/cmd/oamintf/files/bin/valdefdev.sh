#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/valdefdev.sh	1.1.7.2"
#ident  "$Header: valdefdev.sh 2.1 91/09/12 $"

DEV=$1
BDEVICE="bdevice"
BLOCKDEV=
# if not bdevice try and get it using devattr
if [ -b "$DEV" ]
then
	BLOCKDEV="$DEV"
	echo $BLOCKDEV
else
	BLOCKDEV=`/usr/bin/devattr $1 $BDEVICE 2> /dev/null`
	echo $BLOCKDEV
	if test "t$BLOCKDEV" = "t"
	then
		BLOCKDEV="$DEV"
		echo $BLOCKDEV
	fi	
fi
# search vfstab for blockdev, if found success else failure
if test "$BLOCKDEV" = "ALL"
then
	echo "true"
	exit 0
fi

# we dominate /etc/vfstab, only reading it - no privs required
while read fsys dummy
do
	case "$fsys" in
	'#'* | '') 
		continue;;
	'-') 
		continue;;
	esac
	if test "$fsys" = "$BLOCKDEV"
	then
		echo "true"
		exit 0
	fi
done < /etc/vfstab
echo "false"
exit 1
