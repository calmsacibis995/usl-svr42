#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/trap.sh	1.2.1.11"

# main()

. ${SCRIPTS}/common.sh


menu_colors warn
unset RETURN_VALUE
menu -f ${HD_MENUS}/trap.1 -o /tmp/response 2>/dev/null
. /tmp/response

rm /tmp/response

if [ "${RETURN_VALUE}" = "1" ]
then
        logmsg "Interruption.. powerdown" POWERDOWN
else
	exit 0
fi
