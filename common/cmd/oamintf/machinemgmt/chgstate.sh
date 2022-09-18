#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/machinemgmt/chgstate.sh	1.2.8.2"
#ident  "$Header: chgstate.sh 2.0 91/07/12 $"

################################################################################
#	Module Name: chgstate.sh
#	Inputs:
#		$1 - grace period
#		$2 - init state
################################################################################

# Run the process in background, give the interface 5 seconds to exit,
# send all output to /dev/null so as not to confuse fmli

nohup sh <<! > /dev/null 2>&1 &
/usr/bin/sleep 5
cd /
$TFADMIN shutdown -y -g"$1" -i$2
!
