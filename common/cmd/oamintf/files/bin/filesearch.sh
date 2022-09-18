#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/filesearch.sh	1.1.9.2"
#ident  "$Header: filesearch.sh 2.1 91/09/12 $"

# Remember to put search.text into a /tmp file 


fsys="${1}"
days="${2}"

if test  -d ${fsys}
	then
		cd ${fsys}
		flist=`$TFADMIN /usr/bin/find . -mtime +${days} -print`
		if test -z "${flist}"
		then
			echo "
   There are no files older than ${days} days in" `pwd` >/tmp/search.text
			exit 1
		else

echo "\nFILES NOT MODIFIED IN THE LAST ${days} DAYS IN `pwd`:\n
       	 file size   date of
owner   (characters) last access  filename
-----   ------------ ------------ --------" >/tmp/search.text
				echo ${flist} | /usr/bin/sort  | /usr/bin/xargs ls -dl  | /usr/bin/cut -c16-24,30-54,57- >>/tmp/search.text
				echo >>/tmp/search.text
			exit 0
		fi

	else
		exit 1

fi
