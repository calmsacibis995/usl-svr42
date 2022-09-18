#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/getfilesize.sh	1.1.10.2"
#ident  "$Header: getfilesize.sh 2.1 91/09/12 $"

# getfilesize.sh

echo "" >/tmp/file.size

fsys=${1}
nfiles=${2}

cd ${fsys}

a="`/usr/bin/expr 0${nfiles} "*" 10`"
$TFADMIN /usr/bin/du -a  |
	/usr/bin/sort -bnr +0 -1  |
	/usr/bin/sed -n 1,0${a}'s:^[0-9]*	\./:$TFADMIN ls -ldsu :p'  |
	/sbin/sh -  |
	/usr/bin/grep -v '^ *[0-9][0-9]* d'  |
	/usr/bin/sed -n 1,0${nfiles}p  |
	/usr/bin/sort -bnr +5 -6 |
	/usr/bin/cut -c21-29,37- > /tmp/$$filesize

afiles="`/usr/bin/cat /tmp/$$filesize  |  /usr/bin/wc -l  |  /usr/bin/cut -c5-`"

if [ "${nfiles}" -ne "${afiles}" ]
then
	nfiles=${afiles}
	echo "
   There are ${nfiles} files in `pwd`.\n" >/tmp/file.size
fi
echo "\nThe ${nfiles} largest files in `pwd`:\n 
       file size   date of
owner (characters) last access  filename
----- ------------ ------------ --------" >>/tmp/file.size
/usr/bin/cat /tmp/$$filesize >>/tmp/file.size
/usr/bin/rm /tmp/$$filesize 
