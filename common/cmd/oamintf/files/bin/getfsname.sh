#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/getfsname.sh	1.2.4.2"
#ident  "$Header: getfsname.sh 2.0 91/07/12 $"

tmpfile=$2

bdrv=`/usr/bin/devattr ${1} bdevice`
cdrv=`/usr/bin/devattr ${1} cdevice`
pdrv=`/usr/bin/devattr ${1} pathname`
if  [ $bdrv ] 
then ddrive=$bdrv
else if  [ $cdrv ] 
	then ddrive=$cdrv
	else if  [ $pdrv ] 
		then ddrive=$pdrv
		else 	
			echo "   Error - ${1} does not have a device pathname" >>/tmp/$tmpfile
			exit 1
     	     fi
     fi
fi
bddrive=${bdrv}

ndrive="${1} drive"

mounted=`$TFADMIN /sbin/mount | /usr/bin/sed -n "\\;${ddrive};s;^\([^ ]*\) on ${ddrive} .*;\1;p"`

echo $mounted>/tmp/unmnt

if [ -z "${mounted}" ]
then
	echo "   File system ${ndrive} is not mounted." >/tmp/$tmpfile
	echo "" >>/tmp/$tmpfile
	exit 1
fi
