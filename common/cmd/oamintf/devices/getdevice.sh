#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/devices/getdevice.sh	1.1.7.2"
#ident  "$Header: getdevice.sh 2.0 91/07/12 $"
################################################################################
# 	Name: getdevice
#		
#	Desc: Get device choices for remove that doesn't have
#	      '/' or '/usr' or some other mounted file system.
#	
#	Arguments: $1 - device type
################################################################################
onefound=no
for device in `/usr/bin/getdev type=$1 2>/dev/null`
do
	root=`/usr/bin/getdev -a type=dpart mountpt=/`
	usr=`/usr/bin/getdev -a type=dpart mountpt=/usr`

	IFS=" 	,"
	reserved=no
	if [ "$1" = "disk" ]
	then
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
				$TFADMIN mount | /usr/bin/grep ${bdev} > /dev/null 2>&1
				if [ $? -eq 0 ]
				then
					reserved=yes
					break
				fi
			fi
		done
	else
		bdev=`/usr/bin/devattr $device bdevice 2>/dev/null`
		if [ $? -eq 0 ]
		then
			$TFADMIN mount | /usr/bin/grep ${bdev} > /dev/null 2>&1
			if [ $? -eq 0 ]
			then
				reserved=yes
			fi
		fi
	fi
	if [ "$reserved" = no ]
	then
		echo "$device\072\c"; /usr/bin/devattr $device desc
		onefound=yes
	fi
done

if [ "$onefound" = yes ]
then
	exit 0
else
	exit 1
fi
