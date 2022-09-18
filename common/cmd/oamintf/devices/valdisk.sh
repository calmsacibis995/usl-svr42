#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/devices/valdisk.sh	1.3.8.2"
#ident  "$Header: valdisk.sh 2.0 91/07/12 $"

################################################################################
#	Module Name: valdisk.sh
#	
#	Inputs:
#		$1 - group
#		$2 - device
#	
#	Description: Verify that a valid disk was entered.
################################################################################
device=`/usr/bin/devattr $2 alias`
if [ $1 ] 
then
	list=`/usr/bin/listdgrp $1`

	for x in `/usr/bin/getdev type=disk $list 2>/dev/null`
	do
		if [ "x$device" = "x$x" ]
		then
			break
		fi
	done
	if [ "x$device" != "x$x" ]
	then
		exit 1
	fi
fi

if [ "`/usr/bin/getdev type=disk $device`" = "$device" ]
then
	root=`/usr/bin/getdev -a type=dpart mountpt=/`
	usr=`/usr/bin/getdev -a type=dpart mountpt=/usr`

	IFS=" 	,"
	reserved=no
	for dpart in `/usr/bin/devattr $device dpartlist 2>/dev/null`
	do
		if [ $dpart = $usr ] || [ $dpart = $root ] 
		then
			reserved=yes
			break
		fi
		bdev=`/usr/bin/devattr $dpart bdevice 2>/dev/null`
		if [ $? -eq 0 ]
		then
			$TFADMIN mount | grep ${bdev} > /dev/null 2>&1
			if [ $? -eq 0 ]
			then
				reserved=yes
				break
			fi
		fi
	done
	[ "$reserved" = no ] && exit 0
fi

exit 1
