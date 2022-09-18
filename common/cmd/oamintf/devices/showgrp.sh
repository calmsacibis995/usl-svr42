#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/devices/showgrp.sh	1.4.6.2"
#ident  "$Header: showgrp.sh 2.0 91/07/12 $"

################################################################################
#	Module Name: showgrp.sh
#	
#	Description: Determine how many devices and groups
#		     there are with given command attribute.
#		     There must be more than 15 devices and
#		     1 group or the group propmt is not
#		     displayed
#	Inputs:
#		$1 - device type
################################################################################
MINDEVICES=15
MINGROUPS=1

[ `/usr/bin/getdev type=$1 | /usr/bin/wc -l` -ge $MINDEVICES -a `/usr/bin/getdgrp type=$1 | /usr/bin/wc -l` -ge $MINGROUPS ] && exit 0 || exit 1

