#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oampkg:common/cmd/oampkg/setsizecvt/setsizecvt.sh	1.2"
#ident	"$Header:$"

#  This script requires that the request script exports PKGLIST.
#
#  This script will destroy anything not listed in the PKGLIST variable.
#  used to create the space file for a set once packages are selected in
#  the set request script.
#


#
#  Make a copy of the setsize file so that we have something to work with
#
SETSPACE=/tmp/${PKGINST}.space
cp ${SETSIZE}/setsize ${SETSPACE}

#
#  Get rid of pkgname: headers from likes we want to keep.
#
for KEEP_PKG in ${PKGLIST}
	do
ed - ${SETSPACE}<<EOF
1,\$s/$KEEP_PKG://
w
w
q
EOF
	done


#
#  Next, delete any lines from packages we're *not* installing.  We know
#  which ones to delete, because they still have the pkgname: header on
#  the line.  So we delete anything with a : in the line.
#
ed - ${SETSPACE}<<EOF
g/:/d
w
w
q
EOF
