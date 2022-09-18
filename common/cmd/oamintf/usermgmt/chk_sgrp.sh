#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/chk_sgrp.sh	1.2.7.2"
#ident  "$Header: chk_sgrp.sh 2.0 91/07/12 $"

################################################################################
#	Command Name: chk_sgrp
#
# 	Description: This functions is used for validating the Supplementary
#		     Group field in Form.addusr.
#
#	Inputs:
#		$1 - Primary group entered in add user
#		$2 - Supplementary group(s) entered
################################################################################

OK=0
PRMGRP=1
BADGRP=2

test -z "$2" && exit $OK	# Optional field, can be blank


# Create a comma separated list of group names and ids.
/usr/bin/sed -e 's/^\([^:]*\):[^:]*:\([^:]*\).*/\1,\2/p' \
-e '/,[0-9]\{1,2\}$/d' /etc/group > /tmp/$$.sgrp
echo 'other,1' >>/tmp/$$.sgrp

GRPNAME=`/usr/bin/grep "^$1," /tmp/$$.sgrp | /usr/bin/cut -d, -f1`
test ! "$GRPNAME" && GRPNAME=`/usr/bin/grep ",$1$" /tmp/$$.sgrp | /usr/bin/cut -d, -f1`

GRPID=`/usr/bin/grep "^$1," /tmp/$$.sgrp | /usr/bin/cut -d, -f2`
test ! "$GRPID" && GRPID=`/usr/bin/grep ",$1$" /tmp/$$.sgrp | /usr/bin/cut -d, -f2`

# If more than one supplementary group is specified, verify
# that they are not seperarated by blanks.
echo $2 | grep ' ' \
&& /usr/bin/rm -f /tmp/$$.sgrp \
&& exit $BADGRP

# verify that groups entered are valid
for x in `echo $2 | /usr/bin/sed 's/,/ /g'`
do
	if test "$x" = "$GRPNAME" || test "$x" = "$GRPID"
	then
		/usr/bin/rm -f /tmp/$$.sgrp
 		exit $PRMGRP
	fi

	test "$x" = "other" || test "$x" = "1" && continue

	if /usr/bin/grep "^$x," /tmp/$$.sgrp > /dev/null ||
	   /usr/bin/grep ",$x$" /tmp/$$.sgrp > /dev/null
	then
		 continue
	else
		 /usr/bin/rm -f /tmp/$$.sgrp
		 echo $x > /tmp/sgrp
		 exit $BADGRP
	fi
	
done

/usr/bin/rm -f /tmp/$$.sgrp
exit $OK
