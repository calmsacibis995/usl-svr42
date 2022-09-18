#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sa:common/cmd/sa/sa2.sh	1.4.1.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sa/sa2.sh,v 1.1 91/02/28 19:30:57 ccs Exp $"
#	sa2.sh 1.4 of 5/8/89
DATE=`date +%d`
RPT=/var/adm/sa/sar$DATE
DFILE=/var/adm/sa/sa$DATE
ENDIR=/usr/bin
cd $ENDIR
$ENDIR/sar $* -f $DFILE > $RPT
find /var/adm/sa \( -name 'sar*' -o -name 'sa*' \) -mtime +7 -exec rm {} \;
