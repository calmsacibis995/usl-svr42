#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#       Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.
#       Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#         All Rights Reserved

#       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#       UNIX System Laboratories, Inc.
#       The copyright notice above does not evidence any
#       actual or intended publication of such source code.

#       Copyright (c) 1990 UNIX System Laboratories, Inc.
#       Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T
#         All Rights Reserved

#       THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#       UNIX System Laboratories, Inc.
#       The copyright notice above does not evidence any
#       actual or intended publication of such source code.

#!/sbin/sh
#ident	"@(#)fs.cmds:common/cmd/fs.d/lost+found.sh	1.1"

#
# mklost+found
#
# Make a lost+found directory.
#
#!      chmod +x ${file}
USAGE="Usage: mklost+found dir entries"
NARGS=`echo $#`
set -ue
umask 022

root=/
slash=

if [ $NARGS -ne 2 ]
then
	echo "$USAGE" >&2
	exit 1
fi

CNT=${2}
DIR=${1}
if [ ! -d $DIR ]
then
	echo "mklost+found: <$DIR> not directory"  >&2
	exit 1
fi

#
#	Make lost+found directory and populate it according
#	to user request
#
if [ ! -d $DIR/lost+found ]
then
mkdir $DIR/lost+found
fi
cd $DIR/lost+found

n=0
while [ ${n} -lt $CNT ]
do
	> a${n} ; > b${n} ; > c${n} ; > d${n} ; > e${n}
	> f${n} ; > g${n} ; > h${n} ; > i${n} ; > j${n}
	> k${n} ; > l${n} ; > m${n} ; > n${n} ; > o${n}
	> p${n} ; > q${n} ; > r${n} ; > s${n} ; > t${n}
	> u${n} ; > v${n} ; > w${n} ; > x${n} ; > y${n}
	> z${n} ; > A${n} ; > B${n} ; > C${n} ; > D${n}
	if [ ${n} -ne 0 ]
	then
		> E${n} ; > F${n}
	fi
	n=`expr ${n} + 32`
done
if [ $CNT -ne 0 ]
then
	rm *
fi
exit 0

