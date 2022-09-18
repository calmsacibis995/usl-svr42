#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp.admin:printers/printers/add/mkform.sh	1.3.9.4"
#ident  "$Header: mkform.sh 2.0 91/07/12 $"
original=$saveinit
echo name=
echo nrow=1
echo ncol=1
echo from=1
echo fcol=11
echo rows=1
echo columns=14
echo value=$1
echo inactive=TRUE

	index=0
	nrow=3
	ncol=1

	cs=`tput -T $lp_prtype csnm $index 2> /dev/null`
	if grep "^charsets=" $datafile > /dev/null
	then
		grep "^charsets=" $datafile |
		tr "," "[\012*]" > /tmp/charsets.$$
	        if grep "charsets=cs0=" /tmp/charsets.$$ > /dev/null 2>&1
		then
			cs=`grep "charsets=cs0=" /tmp/charsets.$$ | cut -f3 -d"=" 2> /dev/null`
		        saveinit=/dev/null
		fi
	fi 
		
	while [ $? -eq 0 -a -n "$cs" -a $index -lt 64 ]
	do
	    echo "name=cs$index:\nnrow=$nrow\nncol=$ncol"
	    echo "frow=$nrow\nfcol=`expr $ncol + 5`"
	    echo "rows=1\ncolumns=14"
	    echo "value=\`echo cs$index=$cs >> $saveinit;echo $cs\`"
	    echo "scroll=true"
	    index=`expr $index + 1`
	    echo "valid=\`regex -v \"\$F`expr $index + 1`\"" 
	    echo "'^[_a-zA-Z0-9].*$'	'true'"
	    echo "'^.*$'			'false'\`"
	    echo "invalidmsg='Invalid entry: Entry limited to letters, numbers, or underscores.'"
	    echo "lininfo=\"charset:F1\""
	    echo "";
	    if grep "cs$index=" /tmp/charsets.$$ > /dev/null 2>&1
	    then
		cs=`grep "cs$index=" /tmp/charsets.$$ | cut -f2 -d"=" 2> /dev/null`
		saveinit=/dev/null
	    else
	        cs=`tput -T $lp_prtype csnm $index 2> /dev/null`
		saveinit=$original
	    fi
	    ncol=`expr $ncol + 19`
	    if [ $ncol -gt 75 ]
	    then
		ncol=1
		nrow=`expr $nrow + 1`
	    fi
	done
rm -f /tmp/charsets.$$

	exit $index
