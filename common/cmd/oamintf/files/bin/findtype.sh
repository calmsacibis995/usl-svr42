#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/findtype.sh	1.1.8.2"
#ident  "$Header: findtype.sh 2.0 91/07/12 $"
FSYS=`/usr/bin/devattr  ${1} bdevice 2> /dev/null`
if [ ! "$FSYS" ]
then
	FSYS=$1
fi
echo "Since the File System type was unknown" >/tmp/findtype
echo "an attempt was made to identify it." >>/tmp/findtype
echo "" >> /tmp/findtype
/usr/sbin/fstyp ${FSYS} >> /tmp/findtype.out
case $? in
	0) 
		echo "The Possible File System type is:\n" >> /tmp/findtype;
		cat /tmp/findtype.out >> /tmp/findtype;;
	1) 
		echo "However, the file system type could not\nbe identified." >> /tmp/findtype;;
	2) 
		echo "The Possible File System type(s) are:\n" >> /tmp/findtype;
		cat /tmp/findtype.out >> /tmp/findtype;
	  	echo "\nWarning: more than one file system type was\nidentified. If in doubt, use the Check task\nwith the check-only option to uniquely identify\nthe file system type." >>/tmp/findtype
esac
/usr/bin/rm -f /tmp/findtype.out 2>/dev/null
exit 0
