#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FILTER/mode.sh	1.1.1.2"
#ident	"$Header: $"
opt=`/usr/bin/grep "$1" /usr/spool/lp/admins/lp/filter.table | /usr/bin/cut -d: -f9`
if [ "$opt" = "" ]
then	echo ""
	exit
else
	if echo "$opt" | /usr/bin/grep MODES > /dev/null
	then 	
		opt=`echo "$opt" | /usr/bin/sed 's/ //g
					s/,/ /g'`
	
		
		vnum=$2
		set -- "$opt"
		j=1
		for i in $*
		do
			if echo "$i" | /usr/bin/grep MODES > /dev/null
			then	opt=`echo $i | /usr/bin/cut -d"S" -f2 | /usr/bin/cut -d"=" -f1`
				single=`echo $i | /usr/bin/cut -d"-" -f2`
				echo $opt>/usr/tmp/m$j.$vnum
				echo $single>/usr/tmp/s$j.$vnum
				j=`/usr/bin/expr $j + 1`
			fi
		done
	else	exit
	fi
fi
