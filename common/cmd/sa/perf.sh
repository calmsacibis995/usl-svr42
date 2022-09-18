#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sa:common/cmd/sa/perf.sh	1.4.6.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sa/perf.sh,v 1.1 91/02/28 19:30:50 ccs Exp $"
MATCH=`who -r|grep -c "[234][	 ]*0[	 ]*[S1]"`
if [ ${MATCH} -eq 1 ]
then
	su sys -c "$TFADMIN /usr/lib/sa/sadc /var/adm/sa/sa`date +%d`"
fi
