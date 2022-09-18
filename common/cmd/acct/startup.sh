#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)acct:common/cmd/acct/startup.sh	1.7.1.4"
#ident "$Header: $"
#	"startup (acct) - should be called from /etc/rc"
#	"whenever system is brought up"
PATH=/usr/lib/acct:/usr/bin:/usr/sbin
acctwtmp "acctg on" >>/var/adm/wtmp
turnacct on
#	"clean up yesterdays accounting files"
rm -f /var/adm/acct/sum/wtmp*
rm -f /var/adm/acct/sum/pacct*
rm -f /var/adm/acct/nite/lock*
