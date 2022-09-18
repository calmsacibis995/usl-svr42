#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)upgrade:i386/cmd/upgrade/tools/up_cleanup.sh	1.4"
#ident	"$Header: $"

upgrade_cleanup()
{
	UPGRADE_STORE=/var/sadm/upgrade

	#
	# This is a generic procedure to clean up the extraneous patch
	# files used during an upgrade installation.  This procedure
	# should be called by every package that contains patch files.
	# It should be called regardless of type of installation:
	# NEWINSTALL, OVERLAY, or UPGRADE.
	#
	# Usage: /usr/sbin/pkginst/upgrade_cleanup $PKGINST
	#
	# Error conditions include:
	#
	#	No argument: for debugging we'll print a usage message
	#	No files in file list
	#
	# None of these errors should occur, if they do, we'll still
	# return 0, since there is really nothing we can do about it.
	#

	[ "$UPDEBUG" = YES ] && set -x

	PKG=${1}

	PATCH_LOC=/etc/inst/up/patch

	#
	# Look for everything in the patch directory that belongs to
	# this package.  The sort -r is being done so that files in
	# directories are deleted before we try to delete the directory.
	#
	# The directory should always be empty by the time we try to
	# remove it, since they are only used for patch files and each
	# package should clean up after itself.  But, we want to be safe.
	#
	# Since all the *.LIST files are part of the base, when we call
	# this from the base pkg postinstall script, we'd remove all the
	# *.LIST files for the packages that need them later, so we'll
	# explictely grep -v them out of the list.
	#

	grep "^/etc/inst/up/patch.* $PKG" /var/sadm/install/contents |
		cut -d" " -f1  | grep -v "LIST$" | sort -r >/tmp/$$.rm

	for i in `cat /tmp/$$.rm`
	do
		[ -f $i ] && rm -f $i
		[ -d $i ] && rmdir $i >/dev/null 2>&1

		removef $PKG $i >/dev/null 2>&1

		[ "$UPDEBUG" = YES ] && goany
	done

	removef -f $PKG >/dev/null 2>&1
	rm -f /tmp/$$.rm

	[ -f $PATCH_LOC/$PKG.LIST ] && {
	
		rm -f $PATCH_LOC/$PKG.LIST

		# Currently the *.LIST files are part of the base

		removef base $PATCH_LOC/$PKG.LIST >/dev/null 2>&1
		removef -f base >/dev/null 2>&1
	}

	#
	# Now clean up directories left empty.  We're doing this just
	# in case all the relevent directories are not be listed in
	# the contents file.
	#

	find $PATCH_LOC -type d -depth -print | xargs rmdir >/dev/null 2>&1

	[ ! -s  $UPDIR/up.err ] && rm $UPDIR/up.err
	rmdir $UPDIR >/dev/null 2>&1

	rm -f $UPGRADE_STORE/$PKG.env

	[ "$UPDEBUG" = YES ] && goany
}

#Main

PKG=${1}

[ ! "$PKG" ] && {

	echo Usage: $0 pkginst
	exit 0
}

UPDIR=/etc/inst/up
SCRIPTS=/usr/sbin/pkginst
. $SCRIPTS/updebug

[ "$UPDEBUG" = YES ] && set -x

upgrade_cleanup $PKG

exit 0
