#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/valweeks.sh	1.5.7.2"
#ident  "$Header: valweeks.sh 1.2 91/06/21 $"
TABLE=`echo $3`
if [ "$1" = "demand" ]
then
	exit 0
elif [ "$1" = "$2" ]
then
	exit 0
else
	WEEKS=`echo $1`
	PERIOD=`getrpd $TABLE`
	validweeks "$WEEKS" $PERIOD;a=`echo $?`
echo "weeks is $WEEKS period is $PERIOD and retrun validweeks $a" >/tmp/weeks.out
	exit $a
fi
