#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sa:common/cmd/sa/sa1.sh	1.5.1.5"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sa/sa1.sh,v 1.1 91/02/28 19:30:55 ccs Exp $"
#	sa1.sh 1.5.1.3 of 7/27/90

# Privileges:	P_DEV
# Restrictions:
#		cd: none

priv -allprivs work
DATE=`date +%d`
ENDIR=/usr/lib/sa
DFILE=/var/adm/sa/sa$DATE
cd $ENDIR
if [ $# = 0 ]
then
	exec $ENDIR/sadc 1 1 $DFILE
else
	exec $ENDIR/sadc $* $DFILE
fi
