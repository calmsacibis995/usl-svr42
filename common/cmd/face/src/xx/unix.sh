#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)face:src/xx/unix.sh	1.2.4.3"
#ident  "$Header: unix.sh 1.4 91/10/16 $"
gettxt uxface:71 "To return, type \`exit\` or control-d"
gettxt uxface:72 "You are in `pwd`"
exec ${SHELL:-/bin/sh}
