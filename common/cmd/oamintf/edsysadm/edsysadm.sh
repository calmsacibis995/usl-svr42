#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/edsysadm/edsysadm.sh	1.7.6.5"
#ident  "$Header: edsysadm.sh 2.0 91/07/12 $"

################################################################################
#	Module Name: edsysadm
#	Date: July 87
################################################################################

if [ $# -ne 0 ]
then
	echo "	UX edsysadm: invalid syntax."
	echo "	Usage: edsysadm	(NO options or arguments)"
	exit 1
fi
	
OAMBASE=/usr/sadm/sysadm
INTFBASE=$OAMBASE/menu
EDSYSADM=$OAMBASE/edmenu
TESTBASE=${TESTBASE:-/var/tmp}
OBJ_DIR=/usr/sadm/sysadm/edmenu
PATH=/sbin:$OAMBASE/edbin:/usr/bin:/usr/sbin:/etc:/usr/sadm/bin:/usr/sadm/install/bin

export INTFBASE OAMBASE EDSYSADM TESTBASE PATH OBJ_DIR

# make certain we have enough space to build in
# temporary area.

MINSPACE=500
space=`df ${TESTBASE} | cut -d':' -f2 | nawk '{ print $1 }'`
if [ ${space} -lt ${MINSPACE} ]
then
	echo "\tUX edsystem: need ${MINSPACE} blocks in TESTBASE directory \c"
	echo "(${TESTBASE} has ${space} blks.)\n\t\t  Shell environment \c"
	echo "variable TESTBASE overrides default."
	exit 28			#ENOSPC
fi

# Execute fmli with the initialization file and
# top level edsysadm menu

fmli -i $INTFBASE/oam.init -c $INTFBASE/oam.cmd $EDSYSADM/Menu.pkg
