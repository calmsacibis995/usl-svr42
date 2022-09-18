#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/overlaychk.sh	1.25"
#ident	"$Header: $"

vfstab_recreated_screen ()
{
    [ "$UPDEBUG" = YES ] && set -x

    # if $UPGVFS exist, then vfstab on the hard disk was missing or corrupted
    # tell the user that vfstab was recreated. The user chooses between
    # powerdown and continuing with non-destructive installation.

    # we'll need $UPGVFS in Do_upnover_Checks to know whether or the
    # not the filesystems have already been mounted or not.

    cp $UPGVFS $ETC/vfstab
    export os_str
    unset RETURN_VALUE
    menu_colors warn

    [ "$UPDEBUG" = YES ] && goany

    menu -f ${HD_MENUS}/overlaychk.1 -o $UPTMP/$$.resp  2>/dev/null
    . $UPTMP/$$.resp

    [ ${RETURN_VALUE} = 1 ] && {  # do not continue 

       cd /;
       logmsg "Existing other OS. Do not to continue." POWERDOWN
    }

    return
}

Do_upnover_Checks () {

    [ "$UPDEBUG" = YES ] && set -x

    unset UPGERR	#set to MOUNTFAIL or NOSPACE
     
    #run df on the file systems specified in the file $DISK_SPACE
    #set flag for insufficient space for upgrade or overlay install

    [ "$UPDEBUG" = YES ] && goany

    copy_vfstab

    sync; sync;	sync;
     
    # If $UPGVFS exists, the files systems have already mounted.
    # Otherwise, /stand, /usr, and /var, if in $ETC/vfstab, will be mounted
    # UPGERR is set to MOUNTFAIL, in that case

    [ ! -f $UPGVFS ] && mount_other_fs

    [ "$UPGERR" = "MOUNTFAIL" ] && return

    cp $UPGMNTTAB $ETC
     
    [ "$UPDEBUG" = YES ] && goany "mount_other_fs succeeded"
     
    # check free space in / /var and /usr.
    # if not enough space set UPGERR to NOSPACE

    Disk_Free_Space  

    [ "$UPDEBUG" = YES ] && goany "After Disk_free _space"
    sync; sync; sync;
}

copy_vfstab () {

    #create a new /etc/vfstab on the RAM  by copying $ETC/vfstab

    [ "$UPDEBUG" = YES ] && set -x

    [ -f /etc/vfstab ] && mv /etc/vfstab $UPTMP/vfstab

    cp $ETC/vfstab /etc/vfstab

    #pdi naming for DESTINY only. Do not to change any names on a V4 box.

    [ "$UPDEBUG" = YES ] && goany

    [ "$exist_os" = "UNIX_V4" ] && return

    #As a result of the pdi naming convention, the /dev/[r]dsk/c* names
    #on the hard disk for the same minor device may not match those on the 
    #ram disk.
    #map the names /dev/[r]dsk/c* on the hard disk with those on the ram 
    #using the minor device numbers.

    #This is ls -l /dev/dsk/c*. 
    #We will need the minor device number for these nodes.

    for i in /dev/dsk/c*
    do
    	echo $i | cpio -ocv 2>/dev/null | cpio -ivt >> $UPTMP/$$.ram_dsk 2>/dev/null
    done

    [ "$UPDEBUG" = YES ] && goany "ls -l /dev/dsk/c*"

    #This is ls -l /dev/rdsk/c*. 

    for i in /dev/rdsk/c*
    do
    	echo $i | cpio -ocv 2>/dev/null | cpio -ivt >> $UPTMP/$$.ram_rdsk 2>/dev/null
    done

    [ "$UPDEBUG" = YES ] && goany "ls -l /dev/rdsk/c*"

    IFS=$TAB

    #Get the names in the vfstab that need mapping.

    grep "^/dev/dsk/c" /etc/vfstab >$UPTMP/$$.1

    while read special fsck_dev mount_pt fs_type fsck_pass auto_mnt mnt_flgs
    do
        [ "$UPDEBUG" = YES ] && goany </dev/tty

        #Now get the get the corresponding name on the 
        #ram disk with the same minor dev number.
    	find_same_minor   $special  $UPTMP/$$.ram_dsk
    	read x1 x2 x3 x4 x5 x6 x7 x8 x9 x10 DSK <$UPTMP/$$.4

    	find_same_minor   $fsck_dev  $UPTMP/$$.ram_rdsk
    	read x1 x2 x3 x4 x5 x6 x7 x8 x9 x10 RDSK <$UPTMP/$$.4

       	echo \
	"$DSK\t$RDSK\t$mount_pt\t$fs_type\t$fsck_pass\t$auto_mnt\t$mnt_flgs" \
		>>$UPTMP/$$.3

    	IFS=$TAB

    done <$UPTMP/$$.1

    #Get the remaining lines in the vfstab.

    grep -v  "^/dev/dsk/c" /etc/vfstab >$UPTMP/$$.2
    IFS=$SPACE
    >/etc/vfstab

    cat $UPTMP/$$.3 $UPTMP/$$.2 > /etc/vfstab

    [ "$UPDEBUG" = YES ] && goany

    return
}

find_same_minor () {

    [ "$UPDEBUG" = YES ] && set -x

    IFS=$SPACE

    #This is ls -l $HDROOT/dev/[r]dsk/c?t?d?s? 

    echo $HDROOT/$1 | cpio -ocv 2>/dev/null | cpio  -itv >$UPTMP/$$.dsk 2>/dev/null
    read f1 f2 f3 f4 f5 minor f7 f8 f9 f10 f11 <$UPTMP/$$.dsk

    #Get the special dev node with the same minor number on the ram disk.

    grep ",${SPACE}${SPACE}*${minor}${SPACE}" $2 >$UPTMP/$$.4

    [ "$UPDEBUG" = YES ] && goany "end of find_same_minor"
}

mount_other_fs () {

    [ "$UPDEBUG" = YES ] && set -x

    # mount /stand, /usr, and /var

    for FS in /stand /var /usr
    do
	grep "${TAB}${FS}${TAB}" /etc/vfstab >>$UPTMP/$$.FS 2>/dev/null
    done

    IFS=$TAB

    while read special fsck_dev mount_pt fs_type fsck_pass auto_mnt mnt_flgs
    do
        [ "$UPDEBUG" = YES ] && goany </dev/tty

    	instcmd mount -v ${special} $HDROOT/$mount_pt >$UPTMP/$$.mnt 2>/dev/null
    	rc=$?

	read xx fs_time <$UPTMP/$$.mnt

	case $rc in
	   0)  ;;
	  28)  # file system is corrupted
	   [ "$fs_type" ] && {

		if [ "$fs_type" = "vxfs" ]
		then
			/etc/fs/$fs_type/fsck -y -o full $special > /dev/null 2>&1
		else
			/etc/fs/$fs_type/fsck -y $special > /dev/null 2>&1
		fi

		instcmd mount -v $special $HDROOT/$mount_pt \
					> $UPTMP/$$.mnt 2>/dev/null
		rc=$?

		read xx fs_time <$UPTMP/$$.mnt

		[ "$rc" != "0" ] && {

	    		UPGERR=MOUNTFAIL
			break
		}
	   }
	   ;;
	   *)  	UPGERR=MOUNTFAIL
		break
	        ;;
	esac

	echo "$special\t$HDROOT$mount_pt\t$fs_type\trw\t$fs_time" >>$UPGMNTTAB

    done < $UPTMP/$$.FS

    [ "$UPDEBUG" = YES ] && {

	goany

	[ -f $ETC/mnttab ] && {
		echo "$ETC/mnttab constains:"; cp $ETC/mnttab /dev/console
	}

	[ -f $UPGMNTTAB ] && {
		echo "/etc/mnttab constains:"; cp $UPGMNTTAB /dev/console
	}

	goany "end of mount_other_fs"
    }
}

Disk_Free_Space() {

    [ "$UPDEBUG" = YES ] && set -x

    unset UPGERR

    #required free space for various file systems is in Mbytes

    [ $exist_os = UNIX_V4 ] && {

    	rootfs=5120;		#required free Kb (5Mb) in / 
    	usrfs=16384;		#required free KB (16Mb) in /usr
    	varfs=6144;		#required free KB (6Mb) in /var
    }

    [ $exist_os = UNIX_DESTINY ] && {

    	rootfs=4096;		#required free Kb (4.0Mb) in / 
    	usrfs=1024;		#required free KB (1Mb) in /usr
    	varfs=4096;		#required free KB (4Mb) in /var
    }

    [ $exist_os = UNIX_INTR ] && {
    	rootfs=2560;		#required free Kb (2.5Mb) in / 
    	usrfs=0;		#required free KB (0Mb) in /usr
    	varfs=0;		#required free KB (0Mb) in /var
    }

    [ "$UPDEBUG" = YES ] && goany

    for FS in / /var /usr
    do
	#get a list of the the filesystems we need free space on

	grep "${TAB}${FS}${TAB}" /etc/vfstab >>$UPTMP/$$.SPC 2>/dev/null

	[ $? = 0 ] || {

		#the space needed in $FS will have to come from /
		[ $FS = /var ] && rootfs=`expr $rootfs + $varfs`
		[ $FS = /usr ] && rootfs=`expr $rootfs + $usrfs`
	}
    done

    [ "$UPDEBUG" = YES ] && goany

    #$UPTMP/$$.SPC should have entries for / and /var /uar (if they exist)

    IFS=$TAB

    >$UPTMP/spc_msg

    while read special fsck_dev mount_pt fs_type fsck_pass auto_mnt mnt_flgs
    do
        [ "$UPDEBUG" = YES ] && goany </dev/tty

        #df -b returns the number of free kilobytes

    	/sbin/df -b $HDROOT/$mount_pt   >$UPTMP/$$.df.out 2>/dev/null

	#get rid of the column heading line returned by df -b
    	grep -v "Filesystem" $UPTMP/$$.df.out >$UPTMP/$$.avail

    	IFS=$SPACE

    	read FS avail_Kb <$UPTMP/$$.avail

	case $mount_pt in
	   /)	[ $avail_Kb -lt $rootfs ] && {
    		     echo "/ $rootfs $avail_Kb" >>$UPTMP/spc_msg
		     UPGERR=NOSPACE
		}
		;;
	   /var) [ $avail_Kb -lt $varfs ] && {
    		     echo "/var $varfs $avail_Kb" >>$UPTMP/spc_msg
		     UPGERR=NOSPACE
		 }
		;;
	   /usr) [ $avail_Kb -lt $usrfs ] && {
    		     echo "/usr $usrfs $avail_Kb" >>$UPTMP/spc_msg
		     UPGERR=NOSPACE
		 }
		;;
	esac
    	IFS=$TAB

    done < $UPTMP/$$.SPC

    [ "$UPDEBUG" = YES ] && {

	goany

    	[ "$UPGERR" = NOSPACE  ] && {

		echo "insufficient disk space listed in $UPTMP/spc_msg"
		cp $UPTMP/spc_msg /dev/console ; 
		allow_upgrade
		[ $? = 0 ] && unset UPGERR
    	}

	goany
    }
    [ ! -s $UPTMP/spc_msg ] && rm -f $UPTMP/spc_msg
}

#main

. ${SCRIPTS}/common.sh
. ${SCRIPTS}/updebug.sh

# This step is done if an active UNIX partition is detected
#  and the user has selected up-n-over

[ "$INSTALL_TYPE" != UPNOVER ] && exit 0

[ "$UPDEBUG" = YES ] && set -x

UPG_GLOBALS=/tmp/upg_globals
[ -s "$UPG_GLOBALS" ] && . $UPG_GLOBALS

[ "$UPDEBUG" = YES ] && goany "exist_os=$exist_os in overlaychk.sh"

[ "$exist_os" = UNIX_UNK ]  && exit 0

SPACE=" "
TAB="	"
ETC=$HDROOT/etc
UPGMNTTAB=/etc/mnttab

# The previous step 'chkunixrel.sh' could have set UPGERR=MOUNTFAIL
# This happens if mount of root failed or
# when vfstab was recreated and mount of a critical fs failed
# If UPGERR = MOUNTFAIL, nothing more to be done in this step

[ "$UPGERR" = MOUNTFAIL ]  && exit 0

# If $UPGVFS was recreated in the previous step, check if the user
# wishes to continue with a non-destructive or do a powerdown

[ -f $UPGVFS ] && vfstab_recreated_screen

[ "$UPDEBUG" = YES ] && goany

Do_upnover_Checks

echo "UPGERR=\"$UPGERR\"" >> $UPG_GLOBALS

rm -f $UPTMP/$$.*

[ "$UPDEBUG" = YES ] && goany
