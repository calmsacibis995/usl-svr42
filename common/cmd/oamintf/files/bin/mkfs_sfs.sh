#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/mkfs_sfs.sh	1.2.3.2"
#ident  "$Header: mkfs_sfs.sh 2.1 91/09/12 $"

PROTO=$1
DEVICE=$2
BLOCKS=$3
NSECT=$4
NTRACK=$5
BSIZE=$6
FRAGSIZ=$7
LABEL=$8
MOUNTP=$9
LEVEL=$10
MULTI=$12
echo "" > /tmp/make.out
if [ ! -b $DEVICE ]
then
	BDEVICE=`/usr/bin/devattr "$DEVICE" bdevice 2>/dev/null`
else
	BDEVICE=$DEVICE
fi
if [ "$PROTO" != "NULL"  ]
then
	if $TFADMIN  /sbin/mkfs -F sfs -p $PROTO $BDEVICE >/tmp/mkerr$$ 2>&1
	then
		echo "The file system was created successfully." >> /tmp/make.out
	else
		echo "The file system could not be created:\n" >> /tmp/make.out
		cat /tmp/mkerr$$ >>/tmp/make.out
		/usr/bin/rm /tmp/mkerr$$ 2>/dev/null
		exit 1
		
	fi
else
	if  $TFADMIN /sbin/mkfs -F sfs "$BDEVICE" "$BLOCKS" "$NSECT" "$NTRACK" "$BSIZE" "$FRAGSIZ $LEVEL $MULTI"  >/tmp/mkerr$$ 2>&1
	then
		echo "The file system was created successfully." >> /tmp/make.out
	else
		echo "The file system could not be created:\n" >> /tmp/make.out
		cat /tmp/mkerr$$ >>/tmp/make.out
#		/usr/bin/rm /tmp/mkerr$$ 2>/dev/null
		exit 1
	fi
fi

if [ "$LABEL" != "NULL" ]
then
	/sbin/labelit -F sfs "$BDEVICE" "$LABEL" 2>/dev/null
	echo "The new file system has been labelled $LABEL." >> /tmp/make.out
fi

if [ "$MOUNTP" != "" ]
then
	if mount -F sfs $BDEVICE $MOUNTP 2> /dev/null
	then
		echo "The file system has been mounted as $MOUNTP." >> /tmp/make.out
	fi
fi
exit 0
