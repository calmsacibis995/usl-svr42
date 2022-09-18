#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FILTER/pname.sh	1.1.1.2"
#ident	"$Header: $"
echo "any" > /usr/tmp/pname.$1
/usr/bin/ls /usr/spool/lp/admins/lp/printers > /usr/tmp/name$$
if [ -s /usr/tmp/name$$ ]
then
for i in `/usr/bin/cat /usr/tmp/name$$`
do
		echo $i >> /usr/tmp/pname.$1
done
fi
/usr/bin/rm -f /usr/tmp/name$$
