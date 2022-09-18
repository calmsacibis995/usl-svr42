#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rcmds.d/runbackf.sh	1.1"
#ident	"$Header: $"
TMPFILE=$1
if test -s $TMPFILE
then
	shift
	/usr/bin/backup $* "`cat $TMPFILE`" 
fi
rm -f $TMPFILE
