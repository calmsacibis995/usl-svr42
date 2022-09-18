#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:common/cmd/initpkg/rstab.sh	1.1.10.2"
#ident "$Header: rstab.sh 1.2 91/04/26 $"

#	place share(1M) commands here for automatic execution
#	on entering init state 3.
#
#	share [-F fstype] [ -o options] [-d "<text>"] <pathname> <resource>
#	.e.g,
#	share -F rfs -d "/var/news"  /var/news NEWS
