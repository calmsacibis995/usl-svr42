#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/grpchk.sh	1.1.7.2"
#ident  "$Header: grpchk.sh 2.0 91/07/12 $"

#grpchk
# for verifying group exists

(/usr/bin/sed 's/$/:/
s/^\([^:]*\):[^:]*:\([^:]*\).*/\1,\2/p' /etc/group | /usr/bin/sort
) >/tmp/lsgrp

grp=${1}
if /usr/bin/grep "${grp}" /tmp/lsgrp >/dev/null 2>&1
then /usr/bin/rm -f /tmp/lsgrp
     exit 0
else /usr/bin/rm -f /tmp/lsgrp
     exit 1
fi

