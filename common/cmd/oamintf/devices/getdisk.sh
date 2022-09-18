#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/devices/getdisk.sh	1.2.8.2"
#ident  "$Header: getdisk.sh 2.0 91/07/12 $"
################################################################################
# 	Name: getdisk
#		
#	Desc: Get disk choices for partition or remove that doesn't have
#	      '/' or '/usr' or some other mounted file system.
#	
#	Arguments: $1 - device group
################################################################################
onefound=no
if [ "$1" ]
then
	
	list=`/usr/bin/listdgrp $1`
	for disk in `/usr/bin/getdev type=disk $list 2>/dev/null`
	do
		root=`/usr/bin/getdev -a type=dpart mountpt=/`
		usr=`/usr/bin/getdev -a type=dpart mountpt=/usr`

		IFS=" 	,"
		reserved=no
		for dpart in `/usr/bin/devattr $disk dpartlist 2>/dev/null`
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
		if [ "$reserved" = no ]
		then
			echo "$disk\072\c"; /usr/bin/devattr $disk desc
			onefound=yes
		fi
			
	done
else
	for disk in `/usr/bin/getdev type=disk 2>/dev/null`
	do
		root=`/usr/bin/getdev -a type=dpart mountpt=/`
		usr=`/usr/bin/getdev -a type=dpart mountpt=/usr`

		IFS=" 	,"
		reserved=no
		for dpart in `/usr/bin/devattr $disk dpartlist 2>/dev/null`
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
		if [ "$reserved" = no ]
		then
			echo "$disk\072\c"; /usr/bin/devattr $disk desc
			onefound=yes
		fi
	done
fi

if [ "$onefound" = yes ]
then
	exit 0
else
	exit 1
fi
