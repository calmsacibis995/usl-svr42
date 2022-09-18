#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/getdiskuse.sh	1.3.5.2"
#ident  "$Header: getdiskuse.sh 2.1 91/09/12 $"

echo "" > /tmp/disk.use

fslist=`$TFADMIN /sbin/mount  |  /usr/bin/cut -d' ' -f1  |  /usr/bin/sort`
echo "
   FILE SYSTEM USAGE AS OF" `date '+%m/%d/%y %T'` > /tmp/disk.use
echo " 

   File			Free	Total	Percent
   System		Blocks	Blocks	Full
   ------		------	------	-------" >> /tmp/disk.use
for fs in ${fslist}
{
	eval `$TFADMIN /sbin/df -t ${fs}  |
		/usr/bin/sed '	1s/.*): *\([0-9]*\) .*/free=\1/
			2s/[^0-9]*\([0-9]*\) .*/total=\1/'`
	if [ "${total}" -gt 0 ]
	then
		percent=`/usr/bin/expr \( ${total} - ${free} \) \* 100 / ${total}`%
	elif [ "${total}" -eq 0 ]
	then
		percent=0
	else
		percent=
	fi
	if [ `echo $fs | /usr/bin/wc -c` -gt 5 ]
	then
		echo "   ${fs}		${free}	${total}	${percent}" >> /tmp/disk.use
	else
		echo "   ${fs}			${free}	${total}	${percent}" >> /tmp/disk.use
	fi
}
