#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/rsop.sh	1.3.6.2"
#ident  "$Header: rsop.sh 1.2 91/06/21 $"
# shell to invoke rsoper with appropriate options
DDEV=$1
DCHAR="$2"
DLABELS="$3"
JOBIDS="$4"
USERS="$5"
METHOD=$6
ONAME=$7
ODEV=$8
DISP=$9
TFILE=/tmp/rsop$$

OPTS="-d \"$DDEV"
if [ "$DCHAR" != "" ]
then
	OPTS="$OPTS:$DCHAR"
	if [ "$DLABELS" != "" ]
	then
		OPTS="$OPTS:$DLABELS"
	fi
elif [ "$DLABELS" != "" ]
then
	OPTS="$OPTS::$DLABELS"
fi
OPTS=$OPTS\"
if [ "$JOBIDS" != "all" ]
then
	OPTS="$OPTS -j \"$JOBIDS\""
fi
if [ "$USERS" != "all" ]
then
	OPTS="$OPTS -u \"$USERS\""
fi
if [ "$METHOD" != "" ]
then
	OPTS="$OPTS -m $METHOD"
fi
if [ "$DISP" = "yes" ]
then
	OPTS="$OPTS -v"
fi

#echo rsoper $OPTS
eval rsoper $OPTS >$TFILE 2>&1
RC=$?
echo $TFILE
exit $RC
