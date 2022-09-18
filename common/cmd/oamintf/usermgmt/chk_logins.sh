#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/chk_logins.sh	1.3.9.2"
#ident  "$Header: chk_logins.sh 2.1 91/08/28 $"

################################################################################
#	Commad Name: chk_logins
#
# 	This script is used for validating that exist.  Used in
#	Form.addgrp and Form.modgrp2
################################################################################

test -z "$1" && exit 0	# Optional field, can be blank

if [ "$1" = "-gother" ]
then
	/usr/bin/sed -e 's/^\([^:]*\):[^:]*:\([^:]*\).*/\1,\2/p' \
		/etc/passwd > /tmp/$$.logins
	GRP=other
	shift
else
	/usr/bin/sed -e 's/^\([^:]*\):[^:]*:\([^:]*\).*/\1,\2/p' \
		-e '/,[0-9]\{1,2\}$/d' /etc/passwd > /tmp/$$.logins
	GRP=$1
fi

# If more than one login was specified, verify that
# they are not separated by blank spaces.
echo $GRP | grep ' ' \
  && /usr/bin/rm /tmp/$$.logins \
  && exit 1

# verify that groups entered are valid
for x in `echo $1 | /usr/bin/sed 's/,/ /g'`
do

	if /usr/bin/grep "^$x," /tmp/$$.logins > /dev/null ||
	/usr/bin/grep ",$x$" /tmp/$$.logins > /dev/null
	then
		 continue
	else
		/usr/bin/rm -f /tmp/$$.logins
		 echo $x > /tmp/ln
		 exit 1
	fi
	
done

/usr/bin/rm -f /tmp/$$.logins
exit 0
