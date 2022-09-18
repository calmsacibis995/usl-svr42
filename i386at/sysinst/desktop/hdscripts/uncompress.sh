#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/uncompress.sh	1.1.1.13"

# main()
. ${SCRIPTS}/common.sh

mkdir /etc/initprog > /dev/null 2>&1 
mv /etc/sip /etc/initprog/sip > /dev/null 2>&1 
mv /etc/mip /etc/initprog/mip > /dev/null 2>&1 

addswap -s /dev/rdsk/c0t0d0s0 /dev/dsk/c0t0d0s5 5
[ -f /disk4.cpio.Z ] && {
uncompress /disk4.cpio.Z
(cd /; cpio -icdu < /disk4.cpio > /dev/null 2>&1 )
}
[ ! -f /usr/sbin/pkgadd ]  && {
             >/tmp/null_file
	     echo pkgadd is not /disk4.cpio. Get it somehow.
	     sh 
}
rm -f /disk4.cpio > /dev/null 2>&1 
rm -f /disk4.cpio.Z > /dev/null 2>&1
cd /
sync
chmod 500 $INST/pkginstall
   	
chmod 500  /usr/bin/ckyorn
chmod 500  /usr/bin/ckpath
chmod 500  /usr/bin/ckrange
chmod 500  /usr/bin/message
chmod 500  /sbin/filepriv
chmod 500 /usr/sbin/pkgadd
ln /usr/sbin/pkgadd /usr/sbin/pkgask >/dev/null 2>&1
ln -s /etc/fs/s5/fstyp /usr/lib/fs/s5/fstyp 2> /dev/null
sync	
sync	

# Directory test below is true if doing overlay/interrupted
# install -- we saved files from base package to prevent
# them from being overwritten by floppy versions.

[ -d ${BASE_STORE}2 ] && {
	cd ${BASE_STORE}2
	find * -print | cpio -pdumv / 1>/dev/null 2>/dev/null
	stty isig
	cd /
	stty -isig
	rm -rf ${BASE_STORE}2 1>/dev/null 2>&1
	stty isig
}
