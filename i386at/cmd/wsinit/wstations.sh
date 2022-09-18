#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)wsinit:wstations.sh	1.4"
# Please note that if the user adds entry for other type of workstations
# like for example SunRiver and then removes that entry from this file, wsinit
# will not remove devices created for that type of workstations from the filesystem.

/dev/vt	/dev/kd/kd	9
