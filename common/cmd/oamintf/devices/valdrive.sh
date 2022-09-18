#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/devices/valdrive.sh	1.5.7.2"
#ident  "$Header: valdrive.sh 2.0 91/07/12 $"

################################################################################
#	Module Name: valdrive.sh
#	
#	Inputs:
#		$1 - group
#		$2 - type
#		$3 - device
#	
#	Description: Verify a valid device was entered.
################################################################################
device=`/usr/bin/devattr $3 alias`
if [ $1 ] 
then
	list=`/usr/bin/listdgrp $1`

	for x in `/usr/bin/getdev type=$2 $list`
	do
		if [ "x$device" = "x$x" ]
		then
			exit 0
		fi
	done
	exit 1

elif [ "`/usr/bin/getdev type=$2 $device`" = "$device" ]
then
	exit 0

else
	exit 1
fi
