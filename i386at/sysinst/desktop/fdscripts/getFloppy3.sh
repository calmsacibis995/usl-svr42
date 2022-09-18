#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/fdscripts/getFloppy3.sh	1.15.1.1"

# This script verifies that the third floppy has been inserted,
# copies its contents onto the temporary root file system on
# the hard disk, uncompresses the *Z archives on the floppy,
# so that the needed drivers and commands are necessary to
# create the "real" file systems on the hard disk.

# main()
. ${SCRIPTS}/common.sh

menu_colors regular
menu -f ${FD_MENUS}/getFlop3.1 -o /tmp/response 2>/dev/null
cd /
while [ 1 ]
do
   	instcmd mount -r /dev/dsk/f0t /install > /dev/null 2>&1
   	if [ -f /install/LABEL.4.0.dt ];then
   		break
   	fi
   	instcmd umount /install > /dev/null 2>&1
	menu_colors warn
	menu -f ${FD_MENUS}/getFlop3.2 -o /tmp/response 2>/dev/null
done

menu_colors regular
menu -r -f ${FD_MENUS}/copyfiles.1 -o /tmp/response 2>/dev/null
   
mkdir /etc/fs/bfs /usr/sbin /etc/fs/ufs /etc/fs/sfs >/dev/null 2>&1
mkdir /etc/scsi /etc/scsi/format.d > /dev/null 2>&1
cd /install
find . -print | cpio -pdum ${MOUNTED_ROOT}  >/dev/null 2>&1
cd ${MOUNTED_ROOT}
[ -f disk3.cpio.Z ] && uncompress disk3.cpio.Z  >/dev/null 2>&1
[ -f scripts.cpio.Z ] && uncompress scripts.cpio.Z  >/dev/null 2>&1
cpio -icdu < disk3.cpio  >/dev/null 2>&1
rm -f disk3.cpio
mkdir -p ${MOUNTED_ROOT}/${SCRIPTS} 1>/dev/null 2>/dev/null
cd ${MOUNTED_ROOT}/${SCRIPTS}
cpio -icdu < ${MOUNTED_ROOT}/scripts.cpio  >/dev/null 2>&1
rm -f ${MOUNTED_ROOT}/scripts.cpio
cd ${MOUNTED_ROOT}

##### Insert patch here to change perms for disk special files
##### in the template file /etc/scsi/mkdev.d/disk1 and cdrom1
##### (cdrom1 commented out until addt'l investigation occurs)

# create patch command /sbin/patchit from the following here-doc
cat > /sbin/patchit <<!EOF!
sed s/0644/0600/ /etc/scsi/mkdev.d/disk1 > /etc/scsi/mkdev.d/t.t
cp /etc/scsi/mkdev.d/t.t /etc/scsi/mkdev.d/disk1
#sed s/0666/0600/ /etc/scsi/mkdev.d/cdrom1 > /etc/scsi/mkdev.d/t.t
#cp /etc/scsi/mkdev.d/t.t /etc/scsi/mkdev.d/cdrom1
rm -f /etc/scsi/mkdev.d/t.t
installf -c sysutil base - 1>/dev/null 2>&1 <<!FOO!
/etc/scsi/mkdev.d/disk1
/etc/scsi/mkdev.d/cdrom1
!FOO!
installf -f base 1>/dev/null 2>&1
!EOF!
chmod +x /sbin/patchit

# Now edit /etc/inst/scripts/pdiconfig.sh to run patchit (if it
# exists and is executable) to modify the file.  Modify postreboot.sh
# to remove /sbin/patchit

ed ${SCRIPTS}/pdiconfig.sh 1>/dev/null 2>&1 << !EOF!
/Error
a
[ -x /sbin/patchit ] && /sbin/patchit
.
w
w
q
!EOF!

ed ${SCRIPTS}/postreboot.sh 1>/dev/null 2>&1 << !EOF!
1
a
rm -f /sbin/patchit 1>/dev/null 2>&1
.
w
w
q
!EOF!

#### End of patch to get perms on disks corrected in pdimkdev
####

sync
sync
sync
cd /
instcmd umount /install
[ -f ${MOUNTED_ROOT}/menus.cpio.Z ] && {
   uncompress ${MOUNTED_ROOT}/menus.cpio.Z  >/dev/null 2>&1
}
mkdir -p ${MOUNTED_ROOT}/${HD_MENUS} 1>/dev/null 2>/dev/null
(cd ${MOUNTED_ROOT}/${HD_MENUS}; cpio -icdu < ${MOUNTED_ROOT}/menus.cpio) 1>/dev/null 2>/dev/null
rm -f ${MOUNTED_ROOT}/menus.cpio

# register loadable File Systems using /etc/mod_register
/etc/conf/bin/idmodreg

# bail out at this point if not a new install -- we don't want
# to tell the user we're creating file systems.

[ $INSTALL_TYPE = NEWINSTALL ] || { 	
	# skip to setinsttyp.sh to  attempt upgrade/overlay install
	exit 0
}

#
#  About to do mkfs's
#
# But bail out if doing auto installation -- there is a screen being displayed
# that we don't want cleared...

[ "${INSTALL_MODE}" = "AUTOMATIC" ] && exit 0


menu -c 2>/dev/null
PLURAL=""
[ "${SECOND_DISK}" = "Yes" ] && {
	PLURAL="s"
}
echo "Making file systems on your hard disk${PLURAL}.  This will take a few"
echo "minutes.  Please wait..."
