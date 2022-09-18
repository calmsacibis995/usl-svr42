#!/bin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/getdefl.sh	1.2.5.2"
#ident  "$Header: getdefl.sh 2.1 91/08/28 $"

###############################################################################
#	Module:	  retrieves information from /etc/default/useradd file.       #
#	Replaces: use of getusrdefs command with new command defadm	      #
#	Used by:  usermgmt/defaults/Text.dfltok				      #	
###############################################################################

GROUPID=`/usr/bin/defadm useradd GROUPID`
echo "$GROUPID	(Group Name=\c" >/tmp/defaults
GRPNAME=`/usr/bin/grep "^.*:.*:\`defadm useradd GROUPID | /usr/bin/cut -f2 -d=\`:.*" /etc/group | /usr/bin/cut -f1 -d':'`
echo "$GRPNAME)" >>/tmp/defaults
/usr/bin/defadm useradd HOMEDIR SKELDIR SHELL INACT EXPIRE >>/tmp/defaults
[ -d /var/sadm/pkg/audit ] && \
	/usr/bin/defadm useradd AUDIT_MASK >>/tmp/defaults 
[ -d /var/sadm/pkg/audit ] && \
	/usr/bin/defadm useradd DEFLVL >>/tmp/defaults 
/usr/bin/grep -v ERROR /tmp/defaults
