#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)xdm:copyright/copyright.sh	1.2"
microsoft="All rights reserved.\nCopyright (c) Microsoft Corporation, 1987, 1988."
      usl="All rights reserved.\nCopyright (c) 1992 UNIX System Laboratories, Inc."

# remember that lines appear in reverse order

echo "${microsoft}\n${usl}" | $XWINHOME/bin/copyright $1
