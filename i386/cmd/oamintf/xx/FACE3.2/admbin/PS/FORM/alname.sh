#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/admbin/PS/FORM/alname.sh	1.1.1.2"
#ident	"$Header: $"
echo `/usr/bin/grep "^$1:" /usr/vmsys/OBJECTS/PS/FORM/alnames 2>/dev/null | /usr/bin/cut -d' ' -f2` 
