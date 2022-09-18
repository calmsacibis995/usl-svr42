#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)filemgmt:common/cmd/oamintf/files/bin/rmfile.sh	1.1.9.2"
#ident  "$Header: rmfile.sh 2.1 91/09/12 $"

file=${1}

# These /tmp files should have been created during the execution of
# the current task.  No privileges should be required to remove them.

if test -r /tmp/$file
then
	/usr/bin/rm -f /tmp/$file
fi
