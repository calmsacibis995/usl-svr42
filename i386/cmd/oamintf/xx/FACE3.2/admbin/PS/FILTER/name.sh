#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FILTER/name.sh	1.1.1.2"
#ident	"$Header: $"
if [ "$1" = "" ]
then	echo 2
elif	echo "$1" | /usr/bin/grep "[^0-9A-Za-z_]" > /dev/null
then 	echo 3
elif
	grep ":$1:" /usr/spool/lp/admins/lp/filter.table > /dev/null 2>&1
then	
	echo 1
else	echo 0
fi
