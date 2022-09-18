#!/bin/sh

#ident	"@(#)dtadmin:floppy/dtbackup.sh	1.2"
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


# FIX: i18N the usage message
Cmd=`/usr/bin/basename $0`
USAGE="USAGE: $Cmd -s source -t backup_type -h home_dir [ -i ] [ -p ptyname ] \nList of files/folders to backup read from standard input\n"

BKUP_COMPL=0
BKUP_INCR=1
BKUP_FILES=2

unset Ignore Newer Immediate Source Type PtyName Home DoGrep

while getopts is:p:t:h: opt
do
	case $opt in
	i)	Immediate="-v";;
	s)	Source=$OPTARG;;
	p)	PtyName="-M \"EoM %d\" -G $OPTARG";;
	t)	Type=$OPTARG;;
	h)	Home=$OPTARG;;
	\?)	echo $USAGE
		exit 1;;
	esac
done
shift `expr $OPTIND - 1`

if [ -z "$Source" -o -z "$Type" -o -z "$Home" ] 
then
	echo $USAGE
	exit 2
fi

if /sbin/tfadmin -t cpio > /dev/null 2>& 1
then
	CpioCmd="/sbin/tfadmin cpio"
else
	CpioCmd="/usr/bin/cpio"
fi

ListFile=/tmp/FFILES.$$

if [ "$Type" != "$BKUP_FILES" ]
then
		if [ -r $Home/Ignore ]
		then
			DoGrep=yes
			ListFile=/tmp/GFILES.$$
			Ignore="$Home/Ignore"
		fi
fi
if [ "$Type" = "$BKUP_INCR" ]
then
		if [ -r $Home/.lastpartial ]
		then
			Newer="-newer $Home/.lastpartial"
		elif [ -r $Home/.lastbackup ]
		then
			Newer="-newer $Home/.lastbackup"
		fi
fi
	
/usr/bin/xargs -i /usr/bin/find {}  $Newer -print > $ListFile 
if [ -n "$DoGrep" ]
then
	/usr/bin/fgrep -v -f $Ignore < /tmp/GFILES.$$ > /tmp/FFILES.$$
fi
${XWINHOME:-/usr/X}/adm/dtindex -p $$ $Immediate
/usr/bin/rm -f /tmp/?FILES.$$
if [ -n "$Immediate" ]
then
	/usr/bin/cut -f2 < /tmp/flp_index.$$ \
	| grep -v BLOCKS=                    \
	| $CpioCmd -odlucvB -O $Source           \
	>/tmp/bkupout.$$ 2>/tmp/bkuperr.$$
else
	/usr/bin/cut -f2 < /tmp/flp_index.$$ | /usr/bin/grep -v BLOCKS= \
	| $CpioCmd -odlucvB -O $Source $PtyName 2>&1
fi
if [ "$?" = "0" ]
then
	if [ "$Type" = "$BKUP_INCR" ]
	then
		/usr/bin/touch $Home/.lastpartial
	elif [ "$Type" = "$BKUP_COMPL" ]
	then
		/usr/bin/touch $Home/.lastbackup
	fi
fi
/usr/bin/rm -f /tmp/*.$$
exit 0
