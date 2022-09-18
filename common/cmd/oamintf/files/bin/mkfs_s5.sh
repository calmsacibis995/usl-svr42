#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/mkfs_s5.sh	1.2.4.4"
#ident  "$Header: mkfs_s5.sh 2.0 91/07/12 $"
PROTO=$1
BLOCKSIZE=$2
DEVICE=$3
BLOCKS=$4
INODES=$5
LABEL=$6
MOUNTP=$7
echo "" > /tmp/make.out
if [ ! -b $DEVICE ]
then
	BDEVICE=`/usr/bin/devattr "$DEVICE" bdevice 2>/dev/null`
else
	BDEVICE=$DEVICE
fi
if [ "$PROTO" != "NULL"  ]
then
	if  $TFADMIN /sbin/mkfs -F s5 -b $BLOCKSIZE $BDEVICE $PROTO 2>/tmp/mkerr$$
	then
		echo "The file system was created successfully." >> /tmp/make.out
	else
		echo "The file system could not be created:\n" >> /tmp/make.out
		/usr/bin/cat /tmp/mkerr$$ >> /tmp/make.out
		/usr/bin/rm /tmp/mkerr$$ 2>/dev/null
		exit 1
		
	fi
else
	if $TFADMIN /sbin/mkfs -F s5 -b $BLOCKSIZE $BDEVICE $BLOCKS:$INODES 2>/tmp/mkerr$$
	then
		echo "The file system was created successfully." >> /tmp/make.out
	else
		echo "The file system could not be created:\n" >> /tmp/make.out
		/usr/bin/cat /tmp/mkerr$$ >> /tmp/make.out
		/usr/bin/rm /tmp/mkerr$$ 2>/dev/null
		exit 1
	fi
fi
if [ "$LABEL" != "NULL" ]
then
	/sbin/labelit -F s5 "$BDEVICE" "$LABEL" "" 2>/dev/null
	echo "The new file system has been labelled $LABEL." >> /tmp/make.out
fi

if [ "$MOUNTP" != "" ]
then
	if $TFADMIN /sbin/mount -F s5 $BDEVICE $MOUNTP 2> /tmp/mnterr$$
	then
		echo "File system successfully mounted as $MOUNTP." >> /tmp/make.out
	else
		echo "File system could not be mounted as \"$MOUNTP\":" >> /tmp/make.out
		/usr/bin/cat /tmp/mnterr$$ >> /tmp/make.out
		/usr/bin/rm /tmp/mnterr$$ 2>/dev/null
	fi

fi
exit 0
