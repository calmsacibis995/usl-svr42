#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)face:src/vinstall/vdelete.sh	1.1.5.4"
#ident  "$Header: vdelete.sh 1.6 91/10/17 $"
ferror()
{
	echo "$*" ; exit 1
}
set -a

LOGINID=${1}

VMSYS=`sed -n -e '/^vmsys:/s/^.*:\([^:][^:]*\):[^:]*$/\1/p' /etc/passwd`
if [ ! -d "${VMSYS}" ]
then
	gettxt uxface:31 "The value for VMSYS is not set."
	echo;
	exit 1
fi

UHOME=`cat /etc/passwd | grep -s "^$LOGINID:" | cut -f6 -d:`
if [ -z "${UHOME}" ]
then
	gettxt uxface:44"\n${LOGNID}'s home directory has not been retrieved correctly."
	echo;
	exit 1
fi

$VMSYS/bin/chkperm -d -u ${LOGINID} 2>&1 || ferror `gettxt uxface:39"You must be super-user to remove $LOGINID as a FACE user."`

#if grep '^\. \$HOME/\.faceprofile$' ${UHOME}/.profile > /dev/null
if cat ${UHOME}/.profile | grep '^\. \$HOME/\.faceprofile$' > /dev/null
then
	cat ${UHOME}/.profile | grep -v '^\. \$HOME/\.faceprofile$' > /tmp/f.del.$$
	cp /tmp/f.del.$$ ${UHOME}/.profile
fi

exit 0
