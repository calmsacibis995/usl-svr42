#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/chkunixrel.sh	1.26"
#ident	"$Header: $"

Check_UNIX_Rel () {

    [ "$UPDEBUG" = YES ] && set -x

    #	exist_os  is set to "UNIX_UNK"  for a UNIX partition
    #   this routine checks UNIX release and sets exist_os to 
    #	"UNIX_V4"  for version 4
    #	"UNIX_DESTINY"  for Destiny
     	
    # Checking unix system configuration...
     
    Umount		#unmount any mounted filesystems

    # UPTMP=/tmp/upgtmp: is on the ram disk for upnover tmp files
    [ -d $UPTMP ] || mkdir -p $UPTMP

    # mount_rootfs returns 0 for successful mount of root file system

    [ "$UPDEBUG" = YES ] && goany

    mount_rootfs	
    [ "$?" = "0" ] || return  # can't do much if unable to mount root

    #If all goes well, root is mounted on $HDROOT 

    # now check if the system installed is  V4 or DESTiny

    # First check for UNIX Destiny system
     
    [ -f $HDROOT/$UNIX_REL ] && {

    	IFS=$TAB
    	read release version < $HDROOT/$UNIX_REL

    	if [ "$release" = "$RELEASE" ]
	then
	    if [ "$version" = "$VERSION" ] 
	    then
		exist_os=UNIX_DESTINY
		os_str="UNIX SVR4.2 Version 1"
	    else
	       if [ "$version" = "PREINST" ]
	       then
		   exist_os=UNIX_INTR
		   os_str="UNIX INTERRUPTED"
		   return
	       fi
	    fi
	fi
    }

    # if exist_os != UNIX_DESTINY, check for Version 4 system

    V4IDFILE=$ETC/conf/pack.d/kernel/space.c
    V4REL="\"4.0\""
    V4VER="\"4\""
     
    [ -f $V4IDFILE -a $exist_os != UNIX_DESTINY ] && {

    	IFS=$TAB
    	grep '^#define REL' $V4IDFILE  2>/dev/null | read junk release 

    	grep '^#define VER' $V4IDFILE  2>/dev/null | read junk version

    	[ "$release" = "$V4REL" -a "$version" = "$V4VER" ] &&  {

		exist_os=UNIX_V4
		os_str="UNIX SVR4.0 Version 4"
	}
    }

    [ "$UPDEBUG" = YES ] && goany
     
    [ "$UPDEBUG" = YES -a "$exist_os" = UNIX_UNK ] && {
	#sets exist_os to UNIX_DESTINY or UNIX_V4
        choose_install_type
    }

    sync; sync; sync;
}

mount_rootfs () {

    [ "$UPDEBUG" = YES ] && set -x

    IFS=$TAB

    # instcmd mount -v returns 0 if successful and writes fs_type in stdout

    instcmd mount -v /dev/dsk/0s1 $HDROOT >$UPTMP/$$.rootfs 2>/dev/null
    rc=$?

    read fs_type root_time <$UPTMP/$$.rootfs

    case $rc in
       0)  ;;
      28)  # file system is corrupted
	   [ "$fs_type" ] && {

		if [ "$fs_type" = "vxfs" ]
		then
			/etc/fs/$fs_type/fsck -y -o full /dev/dsk/0s1 > /dev/null 2>&1
		else
			/etc/fs/$fs_type/fsck -y /dev/dsk/0s1 > /dev/null 2>&1
		fi

    		instcmd mount -v /dev/dsk/0s1 $HDROOT > $UPTMP/$$.rootfs 2>/dev/null
    		rc=$?

    		read fs_type root_time <$UPTMP/$$.rootfs

    		[ "$rc" != "0" ] && {

			unset fs_type
	    		UPGERR=MOUNTFAIL
		}
    	   }
	   ;;
       *)  unset fs_type ;;
    esac

    #save root fs_type in  $ROOTFSTYPE which is cp'ed to /etc/.fstype later

    [ "$fs_type" ] && {

	echo $fs_type >$ROOTFSTYPE
	echo "/dev/root\t$HDROOT\t$fs_type\trw,suid\t$root_time" >$UPGMNTTAB
    }

    [ "$UPDEBUG" = YES ] && goany "mount_rootfs: rc=$rc"

    return $rc
}

Check_vfstab() {

    # This procedure is called to check if /etc/vfstab is 
    # either missing or corrupted.
    # This procedure gets the slices from the vtoc to recreate the vfstab.
    # The file systems on the various slices are mounted by 'instcmd mount'
    # which writes on stdout the file system type for a successful mount.
    
    [ "$UPDEBUG" = YES ] && set -x

    VFSTAB=$ETC/vfstab

    [ -f $VFSTAB ] ||  unset VFSTAB

    [ "$VFSTAB" ] && {

    	for FS in / /stand
    	do
    		grep "${TAB}${FS}${TAB}" $VFSTAB 1>/dev/null 2>&1

    		[ $? = 0 ] && continue

    		unset VFSTAB
    		break
    	done
    }

    [ "$UPDEBUG" = YES ] && goany

    [ "$VFSTAB" ] && return

    # make vfstab in $UPGVFS
    # the existence of $UPGVFS will later determine whether or not to
    # mount /stand, /usr, /var
    
    # root is already mounted and fs_type for root is known.

    IFS=$TAB
    echo "/dev/root\t/dev/rroot\t/\t$fs_type\t1\tno\t-" >$UPGVFS
    
    DISKS=0

    # is there a second disk? partsize returns 0 , if there is a 2nd disk

    partsize -s /dev/rdsk/c0t1d0s0 1>/dev/null 2>&1

    [ $? = 0 ] && DISKS="0 1"

    for I in `echo $DISKS`
    do
    	# printtvtoc is linked to instcmd

    	printvtoc /dev/rdsk/${I}s0 2>/dev/null | grep MOUNTABLE >$UPTMP/$$.prtv

    	# display format in $UPTMP/$$.prtv is:
    	#	 "slice_#(in hex)<tab>FS<tab>MOUNTABLE"
    
    	IFS=$TAB

    	while read slice FSYS ARG3
    	do

		# </dev/tty required because we're in a while read loop
        	[ "$UPDEBUG" = YES ] && goany </dev/tty

    		DEV=/dev/dsk/c0t${I}d0s${slice}
    		RDEV=/dev/rdsk/c0t${I}d0s${slice}
    		AUTOMOUNT=no
    		unset UNMOUNT MOUNT_PT

    		case $FSYS in
    			V_STAND) 	MOUNT_PT=/stand ;;
    			V_VAR)		MOUNT_PT=/var ;;
    			V_USR)		MOUNT_PT=/usr
    					AUTOMOUNT=yes ;;
    			V_TMP)		MOUNT_PT=/tmp
    					AUTOMOUNT=yes; UNMOUNT=1 ;;
    			V_HOME)		MOUNT_PT=/home

			# tag is HOME for /home and /home2. Check slice no.
					[ "$slice" = c ] && MOUNT_PT=/home2

    					AUTOMOUNT=yes; UNMOUNT=1 ;;
    			V_HOME2)	MOUNT_PT=/home2
    					AUTOMOUNT=yes; UNMOUNT=1 ;;
    			*)		continue ;;
    		esac

    		instcmd mount -v  $DEV ${HDROOT}${MOUNT_PT} >$UPTMP/$$.fs 2>/dev/null
    		rc=$?

    		read FSTYPE fs_time <$UPTMP/$$.fs

    		case $rc in
    		   0)	;;
    		  28)	[ "$FSTYPE" ] && {

    				/etc/fs/$FSTYPE/fsck -y $HDROOT/$DEV
    				instcmd mount -v $DEV $HDROOT/$MOUNT_PT >$UPTMP/$$.fs 2>/dev/null
    				rc=$?
    				read FSTYPE fs_time <$UPTMP/$$.fs

    				# we will take note of the filesystems
    				# successfully fsck'ed.
    			}

    			[ $rc != 0 ] && {
    		      		saveerror $rc
				continue
			}

    			;;

    		   *) 	saveerror $rc
    			continue ;;
    		esac

    		echo "$DEV\t$RDEV\t$MOUNT_PT\t$FSTYPE\t1\t$AUTOMOUNT\t-" >>$UPGVFS
    		# unmount all files systems other than /stand, /usr/, and /var

    		[ "$UNMOUNT" ] && {

    			umount $HDROOT/$MOUNT_PT 1>/dev/null 2>&1 
    			continue
    		}

    		echo "$DEV\t$HDROOT$MOUNT_PT\t$FSTYPE\trw\t$fs_time" >>$UPGMNTTAB

    	done <$UPTMP/$$.prtv

    	[ "$UPDEBUG" = YES ] && goany
    done

    # Now write the lines for proc, /dev/dsk/f0 etc. in $UPGVFS

    Write_other_vfstab_lines  $UPGVFS

    mv $UPGVFS $ETC/vfstab
    
    exist_os=UNIX_UNK

    [ "$UPDEBUG" = YES ] && goany
}

# saveerror has one arg, the mount return code
# It sets UPGERR to MOUNTFAIL for failure to mount a critical file system

saveerror () {

    [ "$UPDEBUG" = YES ] && set -x

    # if UNMOUNT is not set, must be one of the critical file systems 
    # (/stand, /var, /usr). set UPGERR to MOUNTFAIL

    [ "$UNMOUNT" ] || UPGERR=MOUNTFAIL

    [ "$UPDEBUG" = YES ] && goany "saveerror called with rc=$1"
}

Umount() {

    [ "$UPDEBUG" = YES ] && set -x

    for F in stand usr tmp var home home2
    do
   	umount $HDROOT/$F	> /dev/null 2>&1
    done

    umount $HDROOT	> /dev/null 2>&1
    sync; sync; sync

    [ "$UPDEBUG" = YES ] && goany
}

Write_other_vfstab_lines ()
{
    # Do_vfstab will write the fixed vfstab entries in $1

    [ "$UPDEBUG" = YES ] && set -x

    echo "/proc\t-\t/proc\tproc\t-\tno\t-" >>$1
    echo "/dev/fd\t-\t/dev/fd\tfdfs\t-\tno\t-" >>$1
    echo "/dev/dsk/f0t\t/dev/rdsk/f0t\t/install\ts5\t-\tno\t-" >>$1
    echo "/dev/dsk/f1t\t/dev/rdsk/f1t\t/install\ts5\t-\tno\t-" >>$1
    echo "/dev/dsk/f0\t/dev/rdsk/f0\t/install\ts5\t-\tno\t-" >>$1
    echo "/dev/dsk/f1\t/dev/rdsk/f1\t/install\ts5\t-\tno\t-" >>$1

    [ "$UPDEBUG" = YES ] && goany
}

choose_install_type () {    

    while true
    do
       echo "UPDEBUG: you are in debug mode for testing up-n-over"
       echo "Enter 'u' for upgrade 'o' for overlay, 'n' for none (u,o,n) ->\c"
       read XXX
       case $XXX in 
    	u|U)  exist_os=UNIX_V4; return 0 ;;
    	o|O)  exist_os=UNIX_DESTINY; return 0 ;;
    	n|N)  return 1 ;;
    	*)    echo "type o or u or n"; continue	;;
       esac
    done
}

allow_upgrade() {

       while true
       do
	  cp $UPTMP/spc_msg /dev/console
          echo "UPDEBUG:continue with overlay testing? (y,n)"; 
	  read XXX
       	  case $XXX in 
    	    y|Y) return 0 ;;
    	    n|N) return 1 ;;
	    *)	 echo "UPDEBUG: answer y or n"
    		 continue	;;
          esac
       done
}

#main

. ${SCRIPTS}/common.sh
. ${SCRIPTS}/updebug.sh

# This step is done if an active UNIX partition is detected
#  and the user has selected up-n-over

[ "$INSTALL_TYPE" = UPNOVER ] || exit 0

[ "$UPDEBUG" = YES ] && set -x

ETC=$HDROOT/etc
UPTMP=/tmp/upgtmp
[ -d $HISTORY ] || mkdir -p $HISTORY
SPACE=" "
TAB="	"
UPG_GLOBALS=/tmp/upg_globals
UPGMNTTAB=/etc/mnttab
UPGVFS=$UPTMP/vfstab.upg

exist_os=UNIX_UNK	#  Check_UNIX_rel will reset exist_os

[ "$UPDEBUG" = YES ] && goany

Check_UNIX_Rel


[ "$UPEBUG" = YES ] && goany

[ "$exist_os" = UNIX_UNK ] || Check_vfstab

echo "exist_os=\"$exist_os\"" > $UPG_GLOBALS
echo "os_str=\"$os_str\"" >> $UPG_GLOBALS
echo "UPGERR=\"$UPGERR\"" >> $UPG_GLOBALS
echo "UPGVFS=\"$UPGVFS\"" >> $UPG_GLOBALS
echo "UPTMP=\"$UPTMP\"" >> $UPG_GLOBALS
rm -f $UPTMP/$$.*

[ "$UPDEBUG" = YES ] && goany
