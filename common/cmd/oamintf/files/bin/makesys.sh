#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/makesys.sh	1.3.3.2"
#ident  "$Header: makesys.sh 2.0 91/07/12 $"

#!	chmod +x ${file}

set -e

#	With gap == 1 the blocks/cylinder has no effect in the file system.
#	Since most formatting routines lay the sectors out in an optimal
#	way, gap == 1 is a reasonable assumption.
gap=1		# rotational gap size
blockcyl=32	# blocks/cylinder

trap 'exit 0' 1 2 15
flags="-qq -k$$"

#ddrive=${1}
#ndrive=`/usr/lbin/drivename ${ddrive}`

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
			echo "   Error - ${1} does not have a device pathname" >>/tmp/makefs.err
			exit 1
     	     fi
     fi
fi

ndrive="${1} drive"

#l=`/sbin/labelit ${ddrive} 2>/dev/null`
#eval `/usr/lbin/labelfsname "${l}"`


label=${2}
fsname=${3}

fsname=`/usr/bin/expr ${fsname} : '/*\(.*\)'`

ERR=0
if [ -f /${fsname} ]
then
	echo "   A file by that name already exists in the root directory." >>/tmp/makefs.err
	ERR=1
elif [ -d /${fsname} ]
	then
	mounted=`$TFADMIN /sbin/mount | /usr/bin/sed 's; .*;;'`
	if echo "$mounted" | /usr/bin/grep "/${fsname}\$" >>/dev/null
	then
		echo "   A file system with that name is already mounted." >>/tmp/makefs.err
		ERR=1
	else
		echo "   WARNING: A directory of that name already exists." >>/tmp/makefs.err
		echo "            Once this medium is mounted, it will overlay" >>/tmp/makefs.err
		echo "            all files and directories in the original" >>/tmp/makefs.err
		echo "            directory (making them inaccessible) until" >>/tmp/makefs.err
		echo "            this medium is unmounted." >>/tmp/makefs.err
	fi
fi

if [ $ERR -eq 1 ]
then
	echo "   Please choose another name." >>/tmp/makefs.err
#	fsname=$Ofsname
#	label=$Olabel
	exit 1
fi

blocks=`/usr/lbin/spclsize -b ${ddrive}`

#	Funny calculation of inodes gives nice multiple-of-10 values.
inodes=`/usr/bin/expr ${blocks} / 70 \* 10`
halfblocks=`/usr/bin/expr ${blocks} / 2`

#	So  df -t  reports what was requested.
inodes=`/usr/bin/expr ${inodes} + 8`

echo "   Building '${fsname}' file system on '${label}'." >>/tmp/file.make 2>&1
sleep 2

#trap 'exit 9' 1 2 15
#trap '	trap "" 1 2
#	exit 9' 0
#	/usr/bin/cat -s /tmp/$$makefsys;  /usr/bin/rm -f /tmp/$$makefsys;  exit 9' 0

# The echo writes over the beginning of the first block, which mkfs(1M) does not.
# This helps make the medium not recognizable as other than a file system.
echo '                                                            ' >${ddrive}
$TFADMIN /sbin/mkfs -F ${5} ${ddrive} ${blocks}:${inodes} ${gap} ${blockcyl} >>/tmp/file.make 2>&1  ||
	{
	echo "   Error in /sbin/mkfs of '${ddrive}'." >>/tmp/file.make
	exit 7
	}
/sbin/labelit ${ddrive} ${fsname} ${label} >>/tmp/file.make 2>&1  ||  
	{
	echo '   Error in /sbin/labelit for ${ddrive} ${fsname} ${label}' >>/tmp/file.make
	exit 7
	}
sync
#/usr/bin/rm /tmp/$$makefsys

set -e

if [ ! -d /${fsname} ]
then
	/usr/bin/mkdir /${fsname}
fi
$TFADMIN /sbin/mount -F ${5} ${ddrive} /${fsname}  >>/tmp/file.make 2>&1 ||  {
#	/usr/lbin/admerr $0 mount of ${ddrive} on /${fsname} failed.
	echo '   Error in /sbin/mount for ${ddrive} ${fsname} ' >>/tmp/file.make
	exit 7
}

trap "	trap '' 1 2
	cd /;  /usr/lbin/diskumount -n '${ndrive}' ${ddrive} /${fsname}" 0 >>/tmp/file.make 2>&1 || {
	echo '   Error in /usr/lbin/diskumount for ${ndrive} ${ddrive} ${fsname} ' >>/tmp/file.make
	exit 7
}

echo "   Initializing '${fsname}' file system." >>/tmp/file.make
cd /${fsname}
umask 000
/usr/bin/mkdir lost+found
cd lost+found
set +e
#	Populate the lost+found directory to grow it to a usable size.
i=`/usr/bin/expr ${inodes} / 40 + 1`
while [ ${i} -gt 0 ]
do
	/usr/bin/tee ${i}1 ${i}2 ${i}3 ${i}4 ${i}5 ${i}6 ${i}7 ${i}8 ${i}9 ${i}0 </dev/null
	i=`/usr/bin/expr ${i} - 1;  exit 0`	# exit 0 should not be needed, but is.
done
/usr/bin/rm -fr .


if test "${4}" = "yes"
then
	sync
	trap 0
	echo '   Mounted.  DO NOT REMOVE THE MEDIUM UNTIL IT IS UNMOUNTED!' >>/tmp/file.make
fi
