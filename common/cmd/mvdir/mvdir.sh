#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)mvdir:mvdir.sh	1.8.2.4"
#ident  "$Header: mvdir.sh 1.2 91/06/27 $"
catalog=uxsysadm
label=UX:mvdir
if [ $# != 2 ]
then
	pfmt -l $label -g $catalog:1 "Incorrect usage\\n"
	pfmt -l $label -g $catalog:2 -s action "Usage: mvdir fromdir newname\\n"
	exit 2
fi
if [ $1 = . ]
then
	pfmt -l $label -g $catalog:3 "Cannot move '.'\\n"
	exit 2
fi
f=`basename $1`
t=$2
if [ -d $t ]
then
	t=$t/$f
fi
if [ -f $t  -o -d $t ]
then
	pfmt -l $label -g $catalog:4 "%s exists\\n" $t
	exit 1
fi
if [  ! -d $1 ]
then
	pfmt -l $label -g $catalog:5 "%s must be a directory\\n" $1
	exit 1
fi

# *** common path tests: The full path name for $1 must not
# *** 			 be an anchored substring of the full
# ***			 path name for $2

here=`pwd`
cd $1
from=`pwd`
dfrom=`dirname $from`
lfrom=`expr $from : $from`

cd $here
mkdir $t		# see if we can create the directory
if [ $? != 0 ]
then
	exit
fi
cd $t
to=`pwd`
dto=`dirname $to`
cd $here
rmdir $t

a=`expr $to : $from`
if [ $a -eq $lfrom -a  $dfrom != $dto ]
then
	pfmt -l $label -g $catalog:6 "Arguments have common path\\n"
	exit 1
fi
# ***

/usr/bin/mv $1 $t
