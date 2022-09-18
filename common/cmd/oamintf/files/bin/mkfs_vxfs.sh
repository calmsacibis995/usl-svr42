#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/mkfs_vxfs.sh	1.2"

# Copyright (c) 1991, 1992 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
# UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
# LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
# IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
# OR DISCLOSURE.
# 
# THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
# TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
# OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
# EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
# 
#               RESTRICTED RIGHTS LEGEND
# USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
# SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
# (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
# COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
#               VERITAS SOFTWARE
# 4800 GREAT AMERICA PARKWAY, SUITE 420, SANTA CLARA, CA 95054

# Portions Copyright 1992, 1991 UNIX System Laboratories, Inc.
# Portions Copyright 1990 - 1984 AT&T
# All Rights Reserved


BLOCKSIZE=$1
DEVICE=$2
BLOCKS=$3
LABEL=$4
MOUNTP=$5
echo "" > /tmp/make.out
if [ ! -b $DEVICE ]
then
	BDEVICE=`devattr "$DEVICE" bdevice 2>/dev/null`
else
	BDEVICE=$DEVICE
fi
if $TFADMIN /sbin/mkfs -F vxfs -obsize=$BLOCKSIZE $BDEVICE $BLOCKS 2>/tmp/mkerr$$
then
	echo "The file system was created successfully." >> /tmp/make.out
else
	echo "The file system could not be created:\n" >> /tmp/make.out
	/usr/bin/cat /tmp/mkerr$$ >> /tmp/make.out
	/usr/bin/rm /tmp/mkerr$$ 2>/dev/null
	exit 1
fi
if [ "$LABEL" != "NULL" ]
then
	/sbin/labelit -F vxfs "$BDEVICE" "$LABEL" 2>/dev/null
	echo "The new file system has been labelled $LABEL." >> /tmp/make.out
fi

if [ "$MOUNTP" != "" ]
then
	if $TFADMIN mount -F vxfs $BDEVICE $MOUNTP 2> /tmp/mnterr$$
	then
		echo "File system successfully mounted as $MOUNTP." >> /tmp/make.out
	else
		echo "File system could not be mounted as \"$MOUNTP\":" >> /tmp/make.out
		cat /tmp/mnterr$$ >> /tmp/make.out
		/bin/rm /tmp/mnterr$$ 2>/dev/null
	fi

fi
exit 0
