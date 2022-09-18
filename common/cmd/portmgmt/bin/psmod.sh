#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:common/cmd/portmgmt/bin/psmod.sh	1.1.7.2"
#ident  "$Header: psmod.sh 2.0 91/07/13 $"

# psmod
# save the old _pmtab and config script
# if modify port service failed, put the old _pmtab and config script back 

cp /etc/saf/$1/_pmtab /var/tmp/p$3
if [ -f /etc/saf/$1/$2 ]
then
	cp /etc/saf/$1/$2 /var/tmp/c$3
fi
sh /var/tmp/cmd$3 >/var/tmp/err$3 2>/var/tmp/err$3
ret=`echo $?`
if [ -f /var/tmp/c$3 ]
then
	$TFADMIN pmadm -g -p $1 -s $2 -z /var/tmp/c$3
	rm -f /var/tmp/c$3
fi
if [ $ret -ne 0 -a $ret -ne 10 ]
then
	$TFADMIN cp /var/tmp/p$3 /etc/saf/$1/_pmtab
	sacadm -x -p $1 2>/dev/null
fi
rm -f /var/tmp/p$3
exit $ret
