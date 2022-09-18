#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:common/cmd/initpkg/rmountall.sh	1.7.19.2"
#ident "$Header: rmountall.sh 1.3 91/06/28 $"

#	Mount remote resources according to a file system table
#	such as /etc/vfstab.

#	file-system-table format:
#
#	column 1	block special file name of file system
#	column 2	the file system name to be checked by fsck
#	column 3	mount-point directory
#	column 4	file system type (may be column 4)
#	column 5	the option to check the file system
#	column 6	the option for automount
#	column 7	mount flags (rw, ro, etc.)
#	column 8	macceiling when security is installed, fs MAC ceiling
#	White-space separates columns.
#	Lines beginning with "#" are comments.  Empty lines are ignored.
#	a '-' in any field is a no-op.

#!	chmod +x ${file}

if [ $# -lt 1 ]
then
	echo >&2 "Usage:  $0 file-system-table ..."
	exit 1
fi

(
	mldmode >/dev/null 2>&1
	exit ${?}
)
if [	"${?}" -eq "0" ]
then	secure="true"
else	secure="false"
fi

cat $*  |
	while	read spec fsckdev mountp fstyp fsckpass automnt mntflg \
		macceiling ignored
	do
		case ${spec} in
		'#'* | '')	continue;;	#  Ignore comments, empty lines
		'-')		continue;;	#  Ignore no-action lines
		esac
		if [ "${fstyp}" = "rfs" -a "${automnt}" = "yes" ]
		then
			if [ "${mntflg}" = "-" ]
			then	mntflg=rw
			fi

			if [	"${secure}"	= "true"	\
			-a	"${macceiling}"	!= ""	\
			-a	"${macceiling}"	!= "-"	]
			then	lopt=" -l ${macceiling}"
			else	lopt=""
			fi

			$TFADMIN /etc/rfs/rmount -F rfs -o ${mntflg} ${lopt} \
				${spec} ${mountp} >/dev/console 2>&1
		fi

	done
	$TFADMIN /etc/rfs/rmnttry&
