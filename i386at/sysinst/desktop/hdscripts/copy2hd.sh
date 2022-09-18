#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/copy2hd.sh	1.15"
#ident	"$Header :$"

Do_vfstab() {
   VFSTAB=${HDROOT}/etc/vfstab
   echo "/proc	-	/proc	proc	-	no	-" >>$VFSTAB
   echo "/dev/fd	-	/dev/fd	fdfs	-	no	-" >>$VFSTAB
   echo "/dev/dsk/f0t	/dev/rdsk/f0t	/install	s5	-	no	-" >>$VFSTAB
   echo "/dev/dsk/f1t	/dev/rdsk/f1t	/install	s5	-	no	-" >>$VFSTAB
   echo "/dev/dsk/f0	/dev/rdsk/f0	/install	s5	-	no	-" >>$VFSTAB
   echo "/dev/dsk/f1	/dev/rdsk/f1	/install	s5	-	no	-" >>$VFSTAB
}

. ${SCRIPTS}/common.sh
. ${SCRIPTS}/updebug.sh
[ "$UPDEBUG" = YES ] && set -x

mkdir -p ${HDROOT}/var/sadm/install/logs 2> /dev/null

[ -d ${HDROOT}/mnt ]   || mkdir ${HDROOT}/mnt 

# read vfstab from hard disk root (/mnt), copy to /etc
# and prepend mount points with /mnt.
# this is so that "mount" calls work in the while loop
# that changes mount point modes before and after mounting

> /etc/vfstab # don't panic! This is the one on the temporary root file
	      # system

while read bdev rdev mntpt fstyp num flag opts
do
	if [ "${mntpt}" = "/" ]
	then
		echo "${bdev}	${rdev}	${HDROOT}	${fstyp}	${num}	${flag}	${opts}" >> /etc/vfstab
	else
		echo "${bdev}	${rdev}	${HDROOT}${mntpt}	${fstyp}	${num}	${flag}	${opts}" >> /etc/vfstab
	fi
done < ${HDROOT}/etc/vfstab

# Make sure that mount points have correct modes
# before and after mounting.
#
while read mountpt mode
do
	if [ -d ${mountpt} ]
	then
		umount ${mountpt} >/dev/null 2>&1
		RETVAL=$?
		chmod ${mode} ${mountpt} >/dev/null 2>&1
		if [ $RETVAL -eq 0 ]
		then
			mount ${mountpt} >/dev/null 2>/tmp/$$.mnt
			RETVAL=$?
			[ $RETVAL -ne 0 ] && {
				echo "failed to remount $mountpt - return code $RETVAL" >/dev/console
				cp /tmp/$$.mnt /dev/console
				goany
			} 
			chmod ${mode} ${mountpt} >/dev/null 2>&1
			rm -f /tmp/$$.mnt
		fi
	else
		mkdir -p ${mountpt} >/dev/null 2>&1
		chmod ${mode} ${mountpt} >/dev/null 2>&1
	fi
	[ "$UPDEBUG" = YES ] && goany
done <<!ENDOFLIST!
	${HDROOT}/home		0775
	${HDROOT}/home2		0775
	${HDROOT}/stand		0755
	${HDROOT}/tmp		1777
	${HDROOT}/usr		0775
	${HDROOT}/var		0775
	${HDROOT}/var/tmp	1777
!ENDOFLIST!

rm -f /etc/vfstab 1>/dev/null 2>&1

[ -d ${HDROOT}/usr/lib/fs/s5 ] || mkdir -p /usr/lib/fs/s5 2> /dev/null

ln /sbin/mount /etc/mount >/dev/null 2>&1

cd /tmp ; find . -print | cpio -pdmu ${HDROOT}/tmp  > /dev/null 2>&1
cd ${MOUNTED_ROOT}
cp etc/sip ${HDROOT}/stand/sip
cp etc/mip ${HDROOT}/stand/mip
[ -f /tape ] && cp /tape ${HDROOT}/tmp/qt

cd ${MOUNTED_ROOT}; find . -print | cpio -pdmu ${HDROOT}  > /dev/null 2>&1
cd /;  find dev -print | cpio -pd ${HDROOT}  > /dev/null 2>&1

# cp the TIMEZONE file we edited in fdscripts onto the hard disk
mv /TIMEZONE ${HDROOT}/etc/TIMEZONE 1>/dev/null 2>/dev/null

ed ${HDROOT}/etc/mnttab << XXXX  > /dev/null 2>&1
1,\$s/mnt//
1,\$s/\/\//\//g
w
q
XXXX
read rootfs < ${ROOTFSTYPE}
[ "$rootfs" = "ufs" ] && {
      bootcntl -r rootfs=ufs /${HDROOT}/etc/boot
   }
[ "$rootfs" = "vxfs" ] && {
      bootcntl -r rootfs=vxfs /${HDROOT}/etc/boot
   }
[ "$rootfs" = "sfs" ] && {
      bootcntl -r rootfs=sfs /${HDROOT}/etc/boot
   }
disksetup -b /${HDROOT}/etc/boot /dev/rdsk/c0t0d0s0
cp ${ROOTFSTYPE} ${HDROOT}/etc/.fstype
sync

[ $INSTALL_TYPE = NEWINSTALL ] && {
	Do_vfstab
}
[ -d ${HDROOT}/usr/sbin/pkginst ] || mkdir -p ${HDROOT}/usr/sbin/pkginst
cp ${SCRIPTS}/updebug.sh ${HDROOT}/usr/sbin/pkginst/updebug

rm -f  ${HDROOT}/dev/swap
(
ln ${HDROOT}/dev/dsk/c0t0d0s0 ${HDROOT}/dev/dsk/0s0
ln ${HDROOT}/dev/dsk/c0t0d0s1 ${HDROOT}/dev/dsk/0s1
ln ${HDROOT}/dev/dsk/c0t0d0s2 ${HDROOT}/dev/dsk/0s2
ln ${HDROOT}/dev/dsk/c0t0d0s3 ${HDROOT}/dev/dsk/0s3
ln ${HDROOT}/dev/dsk/c0t0d0s4 ${HDROOT}/dev/dsk/0s4
ln ${HDROOT}/dev/dsk/c0t0d0s5 ${HDROOT}/dev/dsk/0s5
ln ${HDROOT}/dev/dsk/c0t0d0s6 ${HDROOT}/dev/dsk/0s6
ln ${HDROOT}/dev/dsk/c0t0d0sa ${HDROOT}/dev/dsk/0s10
ln ${HDROOT}/dev/dsk/c0t0d0sb ${HDROOT}/dev/dsk/0s11
ln ${HDROOT}/dev/dsk/c0t0d0sc ${HDROOT}/dev/dsk/0s12
ln ${HDROOT}/dev/dsk/c0t0d0sd ${HDROOT}/dev/dsk/0s13
ln ${HDROOT}/dev/dsk/c0t1d0s0 ${HDROOT}/dev/dsk/1s0
ln ${HDROOT}/dev/dsk/c0t1d0s1 ${HDROOT}/dev/dsk/1s1
ln ${HDROOT}/dev/dsk/c0t1d0s2 ${HDROOT}/dev/dsk/1s2
ln ${HDROOT}/dev/dsk/c0t1d0s3 ${HDROOT}/dev/dsk/1s3
ln ${HDROOT}/dev/dsk/c0t1d0s4 ${HDROOT}/dev/dsk/1s4
ln ${HDROOT}/dev/dsk/c0t1d0s5 ${HDROOT}/dev/dsk/1s5
ln ${HDROOT}/dev/dsk/c0t1d0s6 ${HDROOT}/dev/dsk/1s6
ln ${HDROOT}/dev/dsk/c0t1d0sa ${HDROOT}/dev/dsk/1s10
ln ${HDROOT}/dev/dsk/c0t1d0sb ${HDROOT}/dev/dsk/1s11
ln ${HDROOT}/dev/dsk/c0t1d0sc ${HDROOT}/dev/dsk/1s12
ln ${HDROOT}/dev/dsk/c0t1d0sd ${HDROOT}/dev/dsk/1s13
sync; sync;
ln ${HDROOT}/dev/dsk/0s1 ${HDROOT}/dev/root
ln ${HDROOT}/dev/dsk/0s2 ${HDROOT}/dev/swap
ln ${HDROOT}/dev/dsk/f0q15dt ${HDROOT}/dev/fd0

ln ${HDROOT}/dev/rdsk/c0t0d0s0 ${HDROOT}/dev/rdsk/0s0
ln ${HDROOT}/dev/rdsk/c0t0d0s1 ${HDROOT}/dev/rdsk/0s1
ln ${HDROOT}/dev/rdsk/c0t0d0s2 ${HDROOT}/dev/rdsk/0s2
ln ${HDROOT}/dev/rdsk/c0t0d0s3 ${HDROOT}/dev/rdsk/0s3
ln ${HDROOT}/dev/rdsk/c0t0d0s4 ${HDROOT}/dev/rdsk/0s4
ln ${HDROOT}/dev/rdsk/c0t0d0s5 ${HDROOT}/dev/rdsk/0s5
ln ${HDROOT}/dev/rdsk/c0t0d0s6 ${HDROOT}/dev/rdsk/0s6
ln ${HDROOT}/dev/rdsk/c0t0d0sa ${HDROOT}/dev/rdsk/0s10
ln ${HDROOT}/dev/rdsk/c0t0d0sb ${HDROOT}/dev/rdsk/0s11
ln ${HDROOT}/dev/rdsk/c0t0d0sc ${HDROOT}/dev/rdsk/0s12
ln ${HDROOT}/dev/rdsk/c0t0d0sd ${HDROOT}/dev/rdsk/0s13
ln ${HDROOT}/dev/rdsk/c0t1d0s0 ${HDROOT}/dev/rdsk/1s0
ln ${HDROOT}/dev/rdsk/c0t1d0s1 ${HDROOT}/dev/rdsk/1s1
ln ${HDROOT}/dev/rdsk/c0t1d0s2 ${HDROOT}/dev/rdsk/1s2
ln ${HDROOT}/dev/rdsk/c0t1d0s3 ${HDROOT}/dev/rdsk/1s3
ln ${HDROOT}/dev/rdsk/c0t1d0s4 ${HDROOT}/dev/rdsk/1s4
ln ${HDROOT}/dev/rdsk/c0t1d0s5 ${HDROOT}/dev/rdsk/1s5
ln ${HDROOT}/dev/rdsk/c0t1d0s6 ${HDROOT}/dev/rdsk/1s6
ln ${HDROOT}/dev/rdsk/c0t1d0sa ${HDROOT}/dev/rdsk/1s10
ln ${HDROOT}/dev/rdsk/c0t1d0sb ${HDROOT}/dev/rdsk/1s11
ln ${HDROOT}/dev/rdsk/c0t1d0sc ${HDROOT}/dev/rdsk/1s12
ln ${HDROOT}/dev/rdsk/c0t1d0sd ${HDROOT}/dev/rdsk/1s13
sync; sync
ln ${HDROOT}/dev/rdsk/0s1 ${HDROOT}/dev/rroot
ln ${HDROOT}/dev/rdsk/0s2 ${HDROOT}/dev/rswap
ln ${HDROOT}/dev/rdsk/f0q15dt ${HDROOT}/dev/rfd0
ln ${HDROOT}/dev/tty00 ${HDROOT}/dev/term/000
ln ${HDROOT}/dev/tty01 ${HDROOT}/dev/term/001
chmod 600 ${HDROOT}/dev/rroot ${HDROOT}/dev/root
) > /dev/null 2>&1

# Now copy saved back package files (we saved them so that boot
# floppy copies wouldn't overwrite them)

[ "$UPDEBUG" = YES ] && goany

[ -d ${HDROOT}/${BASE_STORE} ] && {
	# Coming off an interrupted or overlay install
	# Put base files back that would have been overwritten
	# by floppies...

	cd ${HDROOT}/${BASE_STORE}
	# break some links copied from second floppy that cause problems
	# like su, sulogin, initprivs
	[ "$UPDEBUG" = YES ] && goany
	while read j
	do
		rm -f ${HDROOT}/${j} 1>/dev/null 2>&1
	done < ${SCRIPTS}/intr.links

	# use "m" to make sure owner/perms are OK
	find * -type f -print 2>/dev/null | cpio -pdum $HDROOT 1>/dev/null 2>&1
	[ "$UPDEBUG" = YES ] && goany
	# remove lock file
	rm -f ${UPTMP}/savebase.LIST
	# free up space. Don't allow DEL here.
	cd /
	[ "$UPDEBUG" = YES ] && goany
	stty -isig
	rm -rf ${HDROOT}/${BASE_STORE} 1>/dev/null 2>/dev/null
	[ "$UPDEBUG" = YES ] && goany

	# now save files otherwise overwritten by disk4.cpio
	[ "$UPDEBUG" = YES ] && goany
	mkdir -p ${HDROOT}/${BASE_STORE}2 1>/dev/null 2>&1
	cd ${HDROOT}
	grep -v "^#" ${SCRIPTS}/disk4.list | grep -v "^$" > /tmp/disk4.list
	cat /tmp/disk4.list | cpio -pdu ${HDROOT}/${BASE_STORE}2 1>/dev/null 2>&1
	[ "$UPDEBUG" = YES ] && goany
	stty isig
}
