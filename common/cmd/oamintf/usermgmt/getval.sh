#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/getval.sh	1.1.7.2"
#ident  "$Header: getval.sh 2.0 91/07/12 $"

################################################################################
# This command gets default values of MAXWEEKS and MINWEEKS for
# password when not defined in /etc/shadow. It can be modified for
# login also.
################################################################################

case $2 in
"")
	exit;;
esac
case $1 in
-x)  	

	VAL=`$TFADMIN passwd -s $2 | /usr/bin/sed -n -e "s/^[^ ]*[ ]*[^ ]*[ ]*[^ ]*[ ]*[^ ]*[ ]*\([^ ]*\)[	].*/\1/p" `
	echo ${VAL:-`/usr/bin/defadm passwd MAXWEEKS | cut -f2 -d=`};;
-n) 
	VAL=`$TFADMIN passwd -s $2 | /usr/bin/sed -n -e "s/^[^ ]*[ ]*[^ ]*[ ]*[^ ]*[ ]*\([^ ]*\).*/\1/p" `
	echo ${VAL:-`/usr/bin/defadm passwd MINWEEKS | cut -f2 -d=`};;
esac
