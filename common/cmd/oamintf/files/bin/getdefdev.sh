#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/getdefdev.sh	1.1.6.2"
#ident  "$Header: getdefdev.sh 2.1 91/09/12 $"

echo "ALL"
# We dominate vfstab file, no privs needed to read it
while read fsys dummy 
do
	case $fsys in
	'#'* | '')
		continue;;
        '-') 
		continue
	esac

	echo "${fsys}"
done < /etc/vfstab 
