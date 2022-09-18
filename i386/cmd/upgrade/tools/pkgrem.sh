#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)upgrade:i386/cmd/upgrade/tools/pkgrem.sh	1.2"
SCRIPTS=/usr/sbin/pkginst
. ${SCRIPTS}/updebug
[ "$UPDEBUG" = YES ] && set -x

[ "$1" = "PREINSTALL" ] && {
	CONTENTS=/var/sadm/install/contents
	GREPFILE=/tmp/pkgrem.$$
	PACKAGE=$2
	FILELIST=/tmp/$PACKAGE.list
	echo >$GREPFILE
	echo >$FILELIST
	grep " $PACKAGE " $CONTENTS >> $GREPFILE
	grep " $PACKAGE$" $CONTENTS >> $GREPFILE
	cat $GREPFILE | cut -f1 -d" " | /usr/bin/sort -r > $FILELIST
	[ -s $FILELIST ] && {
		while read filename
		do
			if [ -d "$filename" ]
			then
				rmdir $filename
			else
				rm -f $filename
			fi
		done < $FILELIST
		rm -f $GREPFILE 
		export FILELIST PACKAGE
	}
}
[ "$1" = "POSTINSTALL" ] && {
	PACKAGE=$2
	FILELIST=/tmp/$PACKAGE.list
	[ -s $FILELIST ] && {
		while read filename
		do
			/usr/sbin/removef $PACKAGE $filename > /dev/null 2>&1
		done < $FILELIST
	}
	/usr/sbin/removef -f $PACKAGE
	rm -f $FILELIST
}
