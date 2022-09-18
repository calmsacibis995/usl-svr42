#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)face:src/filecab/fileb/ichexec.sh	1.1.4.3"
#ident  "$Header: ichexec.sh 1.6 91/11/07 $"
I18NNF=`gettxt uxface:57 "not found"`
if [ -z "$1" ]
then
	exit 1
else	if [  -x "$1" ]
	then
		exit 0
	else 	
		#search for the string "PATH=" with any leading blanks
		#or tabs
		grep "^[ 	]*PATH=" /etc/profile > /tmp/pro.$$
		if [  -s /tmp/pro.$$ ]
		then
			PATH=/bin:/usr/bin; export PATH
			. /tmp/pro.$$
		fi
		rm -f /tmp/pro.$$
		cmd="$1"
		if /home/vmsys/bin/pathfind $cmd
		then
			exit 0
		fi
		exit 1
	fi
fi
