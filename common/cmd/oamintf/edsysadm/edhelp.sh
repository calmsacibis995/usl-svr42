#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/edsysadm/edhelp.sh	1.3.5.2"
#ident  "$Header: edhelp.sh 2.0 91/07/12 $"
#
# if EDITOR is not set then default it to /usr/bin/ed.
#

if [ -z "`echo $EDITOR`" ]
then
	EDITOR=/usr/bin/ed
	echo "EDITOR has been set to '`echo $EDITOR`'."
	sleep 2
fi

hfname=`echo ${1} | sed -e 's/  *//'`

if [ -z "$hfname" ] 
then
	$EDITOR "Help"
else
	$EDITOR $1
fi
