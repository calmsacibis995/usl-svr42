#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/defdev.sh	1.1.6.2"
#ident  "$Header: defdev.sh 2.0 91/07/12 $"

# we dominate /etc/vfstab's level
# no privileges required to read from it
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
