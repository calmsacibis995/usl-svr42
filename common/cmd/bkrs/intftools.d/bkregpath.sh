#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/bkregpath.sh	1.1.3.2"
#ident  "$Header: bkregpath.sh 1.2 91/06/21 $"

if [ ! -f /tmp/_bkreg_$1 ]
then
	brfindtab bkreg;
else
	cat /tmp/_bkreg_$1;
fi;