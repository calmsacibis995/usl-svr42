#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/bksadd.sh	1.4.7.2"
#ident  "$Header: bksadd.sh 1.2 91/06/21 $"
# script to add a line to crontab to schedule a backup job.
# Lines are tagged to allow easy identification for later changes
# or removal.
HOUR=`echo "$1" | cut -f1 -d:`
MIN=`echo "$1" | cut -f2 -d:`
DAYS=`echo "$2" | sed -e "s/  */,/g"`
MONTHS=`echo "$3" | sed -e "s/  */,/g"`
TABLE="$4"
NOTIFY="$5"
FILE=/tmp/bkrs$$

if [ "$DAYS" = "all" ]
then
	DAYS=\*
fi

if [ "$MONTHS" = "all" ]
then
	MONTHS=\*
fi

OPTS="-t $TABLE"

if [ "$NOTIFY" = "yes" ]
then
	OPTS="$OPTS -m root"
fi

LINE="$MIN $HOUR * $MONTHS $DAYS backup $OPTS #bksched#"

crontab -l >$FILE
echo "$LINE" >>$FILE
crontab <$FILE 2>/dev/null
RC=$?
rm $FILE
exit $RC
