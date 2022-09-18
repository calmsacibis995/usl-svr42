#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/overlaysav.sh	1.12"
#ident	"$Header: $"


save_basefiles () {

    # Prevent files in base package that are already installed from
    # interrupted install or overlay install from being overwritten
    # by floppies and never replaced if user doesn't select base pkg.

    [ "$UPDEBUG" = YES ] && set -x

    # only do this if OVERLAY or INTERRUPTED destructive installation
    [ "${INSTALL_TYPE}" != "OVERLAY" -a "${INSTALL_TYPE}" != "INTERRUPTED" ] && return

    [ -d $ETC/inst/up ] || mkdir -p $ETC/inst/up

    [ "$UPDEBUG" = YES ] && goany
    BASELIST=${SCRIPTS}/intr.base.list

    # eliminate comment lines from intr.base.list

    filelist=${UPTMP}/base.LIST
    rm -f $filelist
    grep -v "^#" ${BASELIST} >$filelist

    [ "$UPDEBUG" = YES ] && goany
    cd $HDROOT

    [ -f "$filelist" ] && {

	[ "$UPDEBUG" = YES ] && goany

	# We use cpio to copy files in order to copy links -- i.e. cp/ln/mv
	mkdir -p $HDROOT/$BASE_STORE 1>/dev/null 2>/dev/null
    	cat ${filelist} | cpio -pduv ${HDROOT}/${BASE_STORE} 1>/dev/null 2>&1
    }

    [ "$UPDEBUG" = YES ] && goany

}

save_volatilefiles () {

    [ "$UPDEBUG" = YES ] && set -x

    # the volatile file list 'boot.LIST' will be save in $ETC/inst/up

    [ -d $ETC/inst/up ] || mkdir -p $ETC/inst/up

    # eliminate comment lines from boot.LIST

    filelist=${UPTMP}/boot.LIST
    rm -f $filelist
    grep -v "^#" ${SCRIPTS}/boot.LIST >$filelist
    cp ${SCRIPTS}/boot.LIST $ETC/inst/up

    cd $HDROOT

    [ -f "$filelist" ] && {

	[ "$UPDEBUG" = YES ] && goany

    	while read filename
    	do
    		[ -f "$filename" ] && {

    			find $filename -print 2>/dev/null |
    			  cpio -pdmu $HDROOT/$UPGRADE_STORE >/dev/null 2>&1
    		}

    	done < ${filelist}
    }

    [ "$UPDEBUG" = YES ] && goany

    # $ETC/vfstab not included in boot.LIST.
    # Save it in $HDROOT/$UPGRADE_STORE/etc/vfstab

    cp $ETC/vfstab $HDROOT/$UPGRADE_STORE/etc/vfstab

    # Remove special files from /dev for UPGRADE only
    [ "$INSTALL_TYPE" = "UPGRADE" ] && rm -rf $HDROOT/dev >/dev/null 2>&1

    # make a lock file so that 'v' files are saved once only 
    >$UPTMP/savedboot.LIST

    [ "$UPDEBUG" = YES ] && goany
}

merge_vfstab() {

    [ "$UPDEBUG" = YES ] && set -x

    IFS=$TAB

    while read special fsck_dev mount_pt fs_type fsck_pass auto_mnt mnt_flgs
    do
    	grep "^$special" $ETC/vfstab >/dev/null

    	[ "$?" = "1" ] && \
    	echo \
	"$special\t$fsck_dev\t$mount_pt\t$fs_type\t$fsck_pass\t$auto_mnt\t$mnt_flgs" >>$ETC/vfstab

    	[ "$UPDEBUG" = YES ] && goany
		
    done <$HDROOT/$UPGRADE_STORE/etc/vfstab

    >$UPTMP/mrgvfstab

    [ "$UPDEBUG" = YES ] && goany
}

Do_vfstab() {

    [ "$UPDEBUG" = YES ] && set -x

   #Do_vfstab will create $ETC/vfstab

   VFSTAB=$ETC/vfstab
   rm -f $VFSTAB

   echo "/proc	-	/proc	proc	-	no	-" >>$VFSTAB
   echo "/dev/fd	-	/dev/fd	fdfs	-	no	-" >>$VFSTAB
   echo "/dev/dsk/f0t	/dev/rdsk/f0t	/install	s5	-	no	-" >>$VFSTAB
   echo "/dev/dsk/f1t	/dev/rdsk/f1t	/install	s5	-	no	-" >>$VFSTAB
   echo "/dev/dsk/f0	/dev/rdsk/f0	/install	s5	-	no	-" >>$VFSTAB
   echo "/dev/dsk/f1	/dev/rdsk/f1	/install	s5	-	no	-" >>$VFSTAB

   [ "$UPDEBUG" = YES ] && goany
}

#main()

. ${SCRIPTS}/common.sh
. ${SCRIPTS}/updebug.sh

[ $INSTALL_TYPE = NEWINSTALL ] && exit 0

[ "$UPDEBUG" = YES ] && set -x

UPG_GLOBALS=/tmp/upg_globals
[ -s "$UPG_GLOBALS" ] && . $UPG_GLOBALS

TAB="	"
SPACE=" "
ETC=$HDROOT/etc

[ "$UPDEBUG" = YES ] && goany

# $UPTMP/savedboot.LIST will be there if files already saved
# if resuming after interrupt or powerdown, do not save the files again
# don't do this for INTERRUPTED destructive installation.

[ -f $UPTMP/savedboot.LIST -o "$INSTALL_TYPE" = "INTERRUPTED" ] || save_volatilefiles

[ "$UPDEBUG" = YES ] && goany "After save_volatilefiles"

# $UPTMP/savedbase.LIST will be there if already saved files from already
# installed base package to prevent boot floppy versions from
# overwriting them.

[ -f $UPTMP/savedbase.LIST ] || save_basefiles

[ "$UPDEBUG" = YES ] && goany "After save_basefiles"

# Now set installation type to NEWINSTALL if INTERRUPTED installation
# is being done. Our thinking here is that we don't want to deal with
# "merge"-related code in scripts, nor see any prompts from packages
# having to deal with merging. 

[ "${INSTALL_TYPE}" = "INTERRUPTED" ] && {
	INSTALL_TYPE=NEWINSTALL
	AUTOMERGE=NULL
	echo "INSTALL_TYPE=NEWINSTALL" >> ${GLOBALS}
	echo "AUTOMERGE=NULL" >> ${GLOBALS}
	exit 0
}

sync
sync
sync

#if resuming after interrupt, do not remake vfstab

[ -f $UPTMP/mrgvfstab ] || {

    Do_vfstab

    [ "$UPDEBUG" = YES ] && goany "After Do_vfstab"

    merge_vfstab
}

[ "$UPDEBUG" = YES ] && goany

sync
sync
sync
