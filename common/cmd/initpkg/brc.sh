#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:common/cmd/initpkg/brc.sh	1.7.11.2"
#ident "$Header: brc.sh 1.2 91/04/25 $"

if [ -f /etc/dfs/sharetab ]
then
	/usr/bin/mv /etc/dfs/sharetab /etc/dfs/osharetab
fi
if [ ! -d /etc/dfs ]
then /usr/bin/mkdir /etc/dfs
fi
>/etc/dfs/sharetab
# chmod 644 /etc/dfs/sharetab
