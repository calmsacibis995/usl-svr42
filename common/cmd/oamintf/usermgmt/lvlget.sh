#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/lvlget.sh	1.2.1.2"
#ident  "$Header: lvlget.sh 2.0 91/07/12 $"

################################################################
##	Name: lvlget.sh
##	Purpose:  return valid or default
##		  level for $LOGIN
################################################################

if [ $# -lt 2 ]
then
	exit 1
fi
LOGIN=$1
TYPE=$2
[ "$TYPE" != "valid" -a "$TYPE" != "default" ] && exit 1 
[ "$TYPE" = "valid" ] && >/tmp/$VPID.field

LINENUM=0
$TFADMIN logins -hl $LOGIN | while read LINE
do
	LINENUM=`expr $LINENUM + 1`
	[ "$LINENUM" = "1" ] && continue
	LVLN=`lvlname | grep "$LINE\$"`
	F_CNT=`echo "$LVLN" | sed 's/[^:]//g' | wc -c`
	if [ $F_CNT -ge 6 ]
	then
		PRTL=`echo $LVLN | cut -f3 -d':'`
		echo "$PRTL" && [ $LINENUM = 2 ] && [ "$TYPE" = "default" ] &&  exit 0
		echo "$PRTL \c" >>/tmp/$VPID.field && [ $LINENUM = 2 ] && [ "$TYPE" = "default" ] &&  exit 0
	else
		echo "$LINE" &&  [ $LINENUM = 2 ] && [ "$TYPE" = "default" ] &&  exit 0
		echo "$LINE \c" >>/tmp/$VPID.field &&  [ $LINENUM = 2 ] && [ "$TYPE" = "default" ] &&  exit 0
	fi
done
exit 0 
