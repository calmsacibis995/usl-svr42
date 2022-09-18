#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:common/cmd/initpkg/rumountall.sh	1.12.15.2"
#ident "$Header: rumountall.sh 1.3 91/06/28 $"

#	Unmounts remote file resources
#	Note: written to depend on as few commands as possible.

ULIST=/tmp/ulist$$

trap '/usr/bin/rm -f ${ULIST}' 0 1 2 3 15

kill=
while [ $# -ge 1 ]
do
	case "$1" in
	-k )
		if [ ! -x /usr/sbin/fuser ]
		then
			echo >&2 "$0:  WARNING!!!  /usr/sbin/fuser not found."
		else
			kill=yes
		fi
		;;
	* )
		echo >&2 "Usage:  $0 [ -k ]
-k	kill processes with files open in each file system before unmounting."
		exit 1
	esac
	shift
done
#		kill queued mounts
 
/usr/bin/rm -f /etc/rfs/rmnttab
 
if [ ${kill} ]
then
	>${ULIST}
	/sbin/mount  |
		/usr/bin/sort -r  |
		while read fs dummy1 dev mode1 mode2 dummy2
		do
			if [ `echo ${mode1}${mode2} | /usr/bin/grep remote` ]
			then
				echo  "${dev} \c" >>${ULIST}
			fi
		done 
	klist=`/usr/bin/cat ${ULIST}`
	if [ "${klist}" = "" ]
	then
		exit
	fi
	$TFADMIN /usr/sbin/fuser -k ${klist} >/dev/null 2>&1
	for dev in ${klist}
	do
		$TFADMIN /sbin/umount -d ${dev}
	done 
else
	/sbin/mount  |
		/usr/bin/sort -r  |
		while read fs dummy1 dev mode1 mode2 dummy2
		do
			if [ `echo ${mode1}${mode2} | /usr/bin/grep remote` ]
			then
				$TFADMIN /sbin/umount -d ${dev}
			fi
		done 
fi
