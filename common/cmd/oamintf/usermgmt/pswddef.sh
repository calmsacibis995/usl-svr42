#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/pswddef.sh	1.2.7.2"
#ident  "$Header: pswddef.sh 2.0 91/07/12 $"

#pswddef
# for running password command

pwlogin=$1

$TFADMIN /usr/bin/passwd $pwlogin 
