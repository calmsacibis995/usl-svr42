#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/devices/erase/floperase.sh	1.2"
#ident "$Header: floperase.sh 1.1 91/08/29 $"

################################################################################
#	Module Name: floperase.sh
################################################################################

a=`/usr/sadm/sysadm/bin/spclsize $1`

/usr/bin/dd if=/dev/zero of=$1 count=${a} 2>/dev/null
