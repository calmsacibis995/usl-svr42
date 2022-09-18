#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/arch/arch.sh	1.1"
#ident	"$Header: $"
#!/sbin/sh
#       Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved


#! /usr/bin/sh
# This shell script offers compatibility with the SunOS
# command arch. It invokes the uname routine with the -m
# option to retreive the same information.

uname -m
