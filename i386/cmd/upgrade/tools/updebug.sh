#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)upgrade:i386/cmd/upgrade/tools/updebug.sh	1.13"
#ident	"$Header: $"

#
# Used for debugging the upgrade/overlay scripts.  To use debugging,
# set UPDEBUG=YES
#

UPDEBUG=NO
UPERR=/etc/inst/up/up.err

[ ! -d /etc/inst/up ] && {

	mkdir -p /etc/inst/up 2>/dev/null

	# If mkdir fails, we'll reset UPERR

	[ $? != 0 ] && UPERR=/dev/null
}

# Add marker to help identify where the output is coming from.

echo "\nENTERING $0\n" >>$UPERR

goany()
{
	( # run in a subshell so we can turn off the set -x

	set +x

	[ "$1" ] && echo "$1"

	echo "Hit <CR> to continue OR "s" to get shell OR [0-9] to exit \c"

	read ANS

	case $ANS in

		[0-9])	exit $ANS ;;

		s)	/sbin/sh ;;
	esac

	#
	# </dev/console added so goany will work in a "while read VAR" loop
	# otherwise the read will grab half the input and we never stop.
	#
	# The >/dev/console was added becasue -q option to pkgadd redirects
	# stdout to /dev/null.  If we broke out to a shell in that case, we
	# got NO output from commands.
	#

	) </dev/console >/dev/console
}
