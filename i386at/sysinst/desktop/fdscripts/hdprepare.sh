#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/fdscripts/hdprepare.sh	1.1.1.6"
#	Portions Copyright (C) 1990, 1991 Intel Corporation.
# 	Portions Copyright (C) 1990 Interactive Systems Corporation.
# 	All Rights Reserved

# This script allows user to make file system selections if custom
# install chosen. For both custom and auto, the script calls hdscripts.sh
# to create file systems on the hard disk.

# main()
. ${SCRIPTS}/common.sh

# Make sure we only run this script if doing a destructive installation.
[ "$INSTALL_TYPE" = NEWINSTALL ] || exit 0 	# skip to setinsttyp.sh for
					   	# upgrade/overlay install

# load custom_fs.sh and run FileSystem_Setup 
[ $INSTALL_MODE = CUSTOM ] && {
	[ -f /tmp/hdscripts.sh ] && rm -f /tmp/hdscripts.sh
	echo "s5" > ${ROOTFSTYPE}
	. ${SCRIPTS}/custom_fs.sh
	FileSystem_Setup
}

[ -f /tmp/hdscripts.sh ] && {
	[ ${INSTALL_MODE} = CUSTOM ] && {
		# Turn off screen invoked out of partitions.sh
		# when hdscripts.sh is done
		echo "menu -c 2>/dev/null" >> /tmp/hdscripts.sh
	}
	exec sh /tmp/hdscripts.sh
}
