#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)portmgmt:common/cmd/portmgmt/bin/ckfile.sh	1.2.7.2"
#ident  "$Header: ckfile.sh 2.0 91/07/13 $"

# ckfile - validate a file name according to the type
#	   Input: $1 - type of file (e.g. command, device, regular and pipe)
#		  $2 - file name

UNKNOWN=-1	# unknown type
OK=0		# everything is ok
NOTHING=1	# nothing entered
NOTFULLPATH=2	# not full path name
NOTEXIST=3	# not exist
NOTEXEC=4 	# not executable
NOTCHARDEV=5 	# not chacracter device
NOTREG=6 	# not a regular file
NOTPIPE=7 	# not a pipe

# nothing entered
test -z "$1" && exit $NOTHING
if [ "$1" = regular ]
then
	test -z "$2" && exit $OK
else
	test -z "$2" && exit $NOTHING
fi
type=$1
eval set $2
case $1 in
	/*)	;;
	*)	exit $NOTFULLPATH;;
		
esac

lsfile=`ls $1 2>/dev/null`

if test -z "$lsfile"
then
	exit $NOTEXIST
fi

case $type in
	command)
#
# cannot check against -x because the user calls sysadm is not
# the one that will invoke the command
#		if [ -f "$1" -a -x "$1" ]
#
		if [ -f "$1" ]
		then
			exit $OK
		else
			exit $NOTEXEC
		fi;;
	device)
		if [ -c "$1" ]
		then
			exit $OK
		else
			exit $NOTCHARDEV
		fi;;
	regular)
		if [ ! -f "$1" ]
		then
			exit $NOTREG
		else
			exit $OK
		fi;;
	pipe)
		if [ -p "$1" ]
		then
			exit $OK
		else
			exit $NOTPIPE
		fi;;
	*)	exit $UNKNOWN;;
esac
