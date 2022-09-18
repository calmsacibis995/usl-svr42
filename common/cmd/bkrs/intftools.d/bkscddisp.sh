#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/intftools.d/bkscddisp.sh	1.3.6.2"
#ident  "$Header: bkscddisp.sh 1.2 91/06/21 $"
# Script displays the lines from crontab marked by the argument passed in.
# It is used to display the backup schedule and backup reminder schedule
# lines.
# Note: the exit 0 is required to force FMLI to display the error message in
# case the command fails.

(crontab -l | grep \#${1}\# | /usr/sadm/bkup/bin/cron_parse -m $1) 2>&1
exit 0
