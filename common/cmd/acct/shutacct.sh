#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)acct:common/cmd/acct/shutacct.sh	1.6.1.4"
#ident "$Header: $"
#	"shutacct [arg] - shuts down acct, called from /usr/sbin/shutdown"
#	"whenever system taken down"
#	"arg	added to /var/adm/wtmp to record reason, defaults to shutdown"
PATH=/usr/lib/acct:/usr/bin:/usr/sbin
_reason=${1-"acctg off"}
acctwtmp  "${_reason}"  >>/var/adm/wtmp
turnacct off
