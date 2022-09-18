#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:common/cmd/portmgmt/bin/modifypm.sh	1.2.7.2"
#ident  "$Header: modifypm.sh 2.0 91/07/13 $"

# modifypm - modify port monitor information in sactab

SACTAB=/etc/saf/_sactab
TMPFILE1=/var/tmp/$$sactab.1
TMPFILE2=/var/tmp/$$sactab.2

sed "s/^$1:/###$1:/" $SACTAB > $TMPFILE1
$TFADMIN cp $TMPFILE1 $SACTAB
sacadm -x 2>/dev/null

flags=""
if [ "$3" = No ]
then
	flags="x"
fi
if [ $4 = DISABLED ]
then
	flags="d$flags"
fi

tmpline="$1:$2:$flags:$5:$6	#$7"

line=`echo "$tmpline"| sed 's/\\//\\\\\\//g'`

sed "s/###$1:.*$/$line/" $TMPFILE1 > $TMPFILE2  
$TFADMIN cp $TMPFILE2 $SACTAB
sacadm -x 2>/dev/null
rm -f $TMPFILE1 $TMPFILE2
