#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)xcpbackup:backup.sh	1.1.2.3"
#ident  "$Header: backup.sh 1.2 91/07/11 $"

#	Backup script 
#	Options:
#	c - complete backup
#	p - incremental ("partial") backup
#	h - backup history
#	u 