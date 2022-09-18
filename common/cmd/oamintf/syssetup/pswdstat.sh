#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/syssetup/pswdstat.sh	1.1.4.3"
#ident  "$Header: pswdstat.sh 2.0 91/07/12 $"

if [ $# -lt 1 ]
then
	echo "UX:sysadm:$0: argument expected"
	exit 1
fi

STATUS=`$TFADMIN passwd -s $1`
LOGIN=`echo $STATUS | /usr/bin/cut -f1 -d" " 2>/dev/null`
PS=`echo $STATUS | /usr/bin/cut -f2 -d" " 2>/dev/null`
LASTCHG=`echo $STATUS | /usr/bin/cut -f3 -d" " 2>/dev/null`
MIN=`echo $STATUS | /usr/bin/cut -f4 -d" " 2>/dev/null`
MAX=`echo $STATUS | /usr/bin/cut -f5 -d" " 2>/dev/null`
WARN=`echo $STATUS | /usr/bin/cut -f6 -d" " 2>/dev/null`

/usr/bin/printf '   Login:   %-12s\n'	$LOGIN

case $PS in
  PS) pstat="password";;
  LK) pstat="lock";;
  NP) pstat="no password";;
esac

/usr/bin/printf '   Password status:  %s\n' "$pstat"

if [ "$LASTCHG" != "" ]
then
	[ "$LASTCHG"  = "00/00/00" ] && LASTCHG="never"
	/usr/bin/printf '   Last changed on:  %s\n' $LASTCHG
fi

[ "$MIN" = "" ] && MIN=undefined
/usr/bin/printf '   Minimum number of days allowed between password changes:   %s\n' "$MIN"

[ "$MAX" = "" ] && MAX=undefined
/usr/bin/printf '   Maximum number of days the password  is valid:   %s\n' "$MAX"

[ "$WARN" = "" ] && WARN=undefined
/usr/bin/printf '   Number of days for password warning message:   %s\n' "$WARN"
