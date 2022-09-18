#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)face:src/filecab/fileb/basename.sh	1.1.4.3"
#ident  "$Header: basename.sh 1.4 91/10/16 $"
AQQQ=${1-.}
AQQQ=`expr //$AQQQ : "\(.*\)\/$" \| $AQQQ`
BQQQ=`expr //$AQQQ : '.*/\(.*\)'`
expr $BQQQ : "\(.*\)$2$" \| $BQQQ
