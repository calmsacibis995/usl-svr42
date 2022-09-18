#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/modgrp.sh	1.3.9.2"
#ident  "$Header: modgrp.sh 2.1 91/08/19 $"

################################################################################
#	Command Name: modgrp
#
#	Description: This scripts does 3 things: 1) modifies group information
#		     2) changes primary group for specified logins 3) adds
#		     supplementary group status to specified logins.
#
# 	Inputs:		$1 - Group name
#			$2 - New Group name
# 			$3 - group ID
# 			$4 - primary group
# 			$5 - supplementary groups
################################################################################

#   There are two pieces of information about the group entry that can
#   change: name and ID.  Since both have been validated and tested for
#   uniqueness prior to calling this command, we can use the override
#   (-o) option when changing the group info.  However, we do have to test
#   if the name has changed or not.
NEWGRP=$1
if [ $2 = $1 ]
then
	$TFADMIN groupmod -g $3 -o $1 
else
	NEWGRP=$2
	$TFADMIN groupmod -g $3 -o -n $2 $1
fi
if [ $? -ne 0 ]
then
	echo "Error modifying group $1." >&2
	exit 1
fi

# change primary group for specified logins
for x in `echo $4 | /usr/bin/sed 's/,/ /g'`
do
	$TFADMIN usermod -g "$NEWGRP" "$x" 
	if [ $? -ne 0 ]
	then
		echo "Error changing primary group to $NEWGRP for $x." >&2
		exit 1
	fi
done

# change supplementary group members
if [ $5 ]
then
	$TFADMIN addgrpmem -g $NEWGRP `echo $5 | /usr/bin/sed 's/,/ /g'` > /dev/null
	if [ $? -ne 0 ]
	then
		echo "Error changing supplementary group member $5 for $1." >&2
		exit 1
	fi
fi
exit 0
