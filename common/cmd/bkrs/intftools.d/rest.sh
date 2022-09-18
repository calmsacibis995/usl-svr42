#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/rest.sh	1.8.7.3"
#ident  "$Header: rest.sh 1.2 91/06/21 $"
# script sets up command line and calls restore utility
TYPE="$1"
NAMES="$2"
TARGET="$3"
DATE="$4"
DISPLAY=$5
NOTIFY=$6
TRACE=$7
TFILE=$8
OPTS=
TEMPF=/tmp/rest_$$

if [ "$TARGET" != "" ]
then
	OPTS="$OPTS -o $TARGET"
fi

OPTS="$OPTS -d \"$DATE\""

if [ "$DISPLAY" = "yes" ]
then
	OPTS="$OPTS -n"
fi

if [ "$NOTIFY" = "yes" ]
then
	OPTS="$OPTS -m"
fi

if [ "$TRACE" = "yes" ]
then
	OPTS="$OPTS -v"
fi

if [ "$TYPE" = "file" ]
then
	CMD=/usr/sbin/urestore
	OPTS="$OPTS -F"
elif [ "$TYPE" = "directory" ]
then
	CMD=/usr/sbin/urestore
	OPTS="$OPTS -D"
elif [ "$TYPE" = "file system" ]
then
	CMD=/usr/sbin/restore
	OPTS="$OPTS -S"
elif [ "$TYPE" = "data partition" ]
then
	CMD=/usr/sbin/restore
	OPTS="$OPTS -P"
else
	echo "Error - type of restore must be file, directory, file system or"
	echo "data partition."
	exit 1
fi

echo "Attempting automatic restore from online archive...\n"
eval $CMD $OPTS "$NAMES" >$TFILE
RC=$?
grep -v "Attempting" $TFILE > $TEMPF 2>/dev/null
mv $TEMPF $TFILE >/dev/null 2>&1
#echo $TFILE
exit $RC
