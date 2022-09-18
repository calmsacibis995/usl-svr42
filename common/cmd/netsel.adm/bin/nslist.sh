#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)netsel.adm:bin/nslist.sh	1.1.7.4"
#ident  "$Header: nslist.sh 2.1 91/09/16 $"

#nslist - display network selection information

UNKNOWN=-1	# unknown type
OK=0		# everything is ok
NOTHING=1	# nothing entered

echo "Network Id    \tNetwork Device     \tSemantics\tProtoFamily\t\b\b\b\bVisible"
echo "--------------\t-------------------\t---------\t-----------\t\b\b\b\b-------"
/usr/bin/cat /etc/netconfig | /usr/bin/grep -v "^#" |
while read nid semantics flags protofamily proto device libraries
do
	if [ ! -z "$nid" ]
	then
		if [ `echo $nid | /usr/bin/wc -c` -ge 9 ];
		then
			echo "$nid\t$device\t\t$semantics\t$protofamily\t\\c"
		else
			echo "$nid\t\t$device\t\t$semantics\t$protofamily\t\\c"
		fi;
		if [ $flags = "-" ]
		then
			echo "no\t\\c"
		else
			echo "yes\t\\c"
		fi
		echo ""
	fi
done
exit 0
# The following is not well defined, right now
	if [ ${protofamily} = "-" ]
	then
		echo "none\t\\c"
	else
		echo "${protofamily}\t\\c"
	fi
	if [ $proto = "-" ]
	then
		echo "none\t\\c"
	else
		echo "$proto\t\\c"
	fi
	if [ $libraries = "-" ]
	then
		echo "none"
	else
		echo "$libraries"
	fi
