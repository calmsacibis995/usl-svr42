#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/valmonths.sh	1.1.6.2"
#ident  "$Header: valmonths.sh 1.2 91/06/21 $"
if [ "$1" = "all" ]
then
	exit 0
else
	MONS=`echo "$1" | sed -e "s/  */,/g"`
	validmons $MONS
	exit $?
fi

