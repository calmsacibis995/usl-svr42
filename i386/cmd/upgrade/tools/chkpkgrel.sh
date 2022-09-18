#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)upgrade:i386/cmd/upgrade/tools/chkpkgrel.sh	1.27"
#ident	"$Header: $"
#
#  This script is invoked from the request script of a package, therefore
#  $PKGINST must be defined. The script is optionally called with one arg.
#  If there is an arg, then it checks the version of $1
#
#  It checks if the $PKGINST or $1 exists on the system
#  If so, it checks its VERSION.  It  exits with code:
#	4, if $PKGINST (or $1) is a Version 4 pkg
#       2, if $PKGINST (or $1) is a DESTiny pkg
#       1, if $PKGINST (or $1) is neither a Version 4 nor a DESTiny pkg
#       0, if $PKGINST (or $1) is not installed or  other problems
#
#

SBINPKGINST=/usr/sbin/pkginst

. $SBINPKGINST/updebug

[ "$UPDEBUG" = "YES" ] && set -x

PKGVERSION=0		#pkg in not installed

# exit 0, if the user selected destructive install
# That is, INSTALL_TYPE=NEWINSTALL in /var/sadm/upgrade/install_type
[ -f /var/sadm/upgrade/install_type ] && {
	. /var/sadm/upgrade/install_type
	[ "$INSTALL_TYPE" = NEWINSTALL ] && exit $PKGVERSION
}

pkg=$PKGINST

[ "$1" ] && {
	pkg="$1"	#check existence/version of pkg $1
	VERSION=1 	#We'll count on SVR4.2 version of the pkg being
			#checked to be 1
}

UPDIR=/etc/inst/up
[ -d $UPDIR ] || mkdir -p $UPDIR

UPGRADE_STORE=/var/sadm/upgrade
[ -d $UPGRADE_STORE ] || mkdir -p $UPGRADE_STORE

PKGINFO=/var/sadm/pkg/$pkg/pkginfo

# if $PKGINFO does not exist, this must be a new installation or
# a version of the package which did not support 'pkginfo' is installed.
# For all these cases we exit with code 0, implying destructive installation


[ "$UPDEBUG" = "YES" ] && goany

[ -f $PKGINFO ] || exit $PKGVERSION

verline=`grep "^VERSION=" ${PKGINFO} 2>>$UPERR`
# save IFS and restore it later. sh does not like '=' as IFS
OIFS=$IFS; IFS="="
set $verline
version=$2
IFS=$OIFS

PKGVERSION=1		#unknown version
[ "$version" ] && {
	# $VERSION is the version being installed.
	# $version is the installed version

	[ "$version" = "$VERSION" ] && {

		# SVR4.2 package

		# Return code 2 for OVERLAY  only if the pkg 
		# is completely installed, 

		PKGVERSION=2 	
	}

	[ "$version" != "$VERSION" -a "$version" = "4" ] && PKGVERSION=4 # V4

	# If /tmp/$pkg.Lock exists, the pkg is only partially installed
	# and the pkg is installed via set installation.
	# Reset the return code to 1,  for new installation of the pkg,
	# if version is checked for the pkg being installed.

	[ -f /tmp/$pkg.Lock -a "$pkg" = "$PKGINST" ] && {
		rm -f /tmp/$pkg.Lock
		PKGVERSION=1
	}
}
	

# create $UPGFILE so that pkgsavfiles.sh knows not to find pkg version again
UPGFILE=$UPGRADE_STORE/$PKGINST.env

# saving $PKGVERSION in $UPGFILE will enable to set PKGINSTALL_TYPE
# in pkgsavfiles, if it is not set already.

[ "$pkg" = "$PKGINST" ] && echo $PKGVERSION >"$UPGFILE"

[ "$UPDEBUG" = "YES" ] && goany

exit $PKGVERSION
