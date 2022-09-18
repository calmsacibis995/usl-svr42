#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/addgrp.sh	1.2.9.3"
#ident  "$Header: addgrp.sh 2.1 91/08/19 $"


################################################################################
#	Command Name: addgrp
#
#	Description: This scripts does 3 things:
#			1) adds group to the system
#		     	2) changes primary group for specified logins
#			3) adds supplementary group status to specified logins.
#
# 	Inputs:		$1 - Group name
# 			$2 - group ID
# 			$3 - primary group(s)
# 			$4 - supplementary group(s)
################################################################################

NOADD=1
BADPRIM=2
BADSUP=3

# add group to system
$TFADMIN groupadd -g $2 $1 2> /tmp/gadderr || exit $NOADD

# change primary group for specified logins
if [ $3 ]
then
	pargs="`echo $3 | /usr/bin/sed 's/,/ /g'`"
	for PLOGIN in $pargs
	do
	   P=`$TFADMIN logins -ol"$PLOGIN" 2>/dev/null | /usr/bin/cut -f1 -d: 2>/dev/null`
	   if [ "$P" != "$PLOGIN" ]
	   then
	       PNAME="`/usr/bin/cat /etc/passwd | /usr/bin/sed -e \"/^[^:]\{1,32\}:[^:]\{1,15\}:$PLOGIN:/p\" -e \"d\" | /usr/bin/cut -f1 -d:`"
	   else
	       PNAME="$PLOGIN"
	   fi
	   $TFADMIN usermod -g "$1" "$PNAME" > /dev/null 2>> /tmp/gadderr \
	   || exit $BADPRIM
	done
fi

# add supplementary group members
if [ $4 ]
then
	args="`echo $4 | /usr/bin/sed 's/,/ /g'`"
	for LOGID in $args
	do
	   A=`$TFADMIN logins -ol"$LOGID" 2>/dev/null | /usr/bin/cut -f1 -d: 2>/dev/null`
	   if [ "$A" != "$LOGID" ]
	   then 
		LNAME="$LNAME `/usr/bin/cat /etc/passwd |\
		/usr/bin/sed -e \"/^[^:]\{1,32\}:[^:]\{1,15\}:$LOGID:/p\" -e \"d\" |\
		 /usr/bin/cut -f1 -d:`"
	   else
		LNAME="$LNAME $LOGID"
	   fi
	done
	$TFADMIN addgrpmem -g $1 $LNAME || exit $BADSUP
fi
