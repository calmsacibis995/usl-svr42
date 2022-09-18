#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/setinsttyp.sh	1.1.1.24"
#ident	"$Header: $"


Set_Install_Type () {

    [ "$UPDEBUG" = YES ] && set -x

    INSTALL_TYPE=NEWINSTALL
    unset RETURN_VALUE

    [ "$UPGERR" = MOUNTFAIL ] && exist_os=UNIX_UNK

    case $exist_os in
	UNIX_UNK)
		menu_colors warn

		[ "$UPDEBUG" = YES ] && goany

  	    	menu -f ${HD_MENUS}/setinst.1 -o $UPTMP/$$.resp  2>/dev/null
	    	. $UPTMP/$$.resp

            	[ ${RETURN_VALUE} = 1 ] && {  # do not continue 
      	        	cd /;
                	logmsg "Existing other OS. Do not to continue." POWERDOWN
   	    	}

   	    	return
	        ;;
        UNIX_INTR)	[ ! "$UPGERR" ] && {
				INSTALL_TYPE=INTERRUPTED
				return	
			}
			# should only fail because there wasn't space
			# to save files from the base packages that
			# would be overwritten when we copy files from
			# the boot floppies.
			# create lock file to force reinstall of base
			# package since there wasn't room to save
			> $HDROOT/var/sadm/pkg/base/!I-Lock!

			# set INSTALL_TYPE to RENEWINSTALL -- indicating
			# that interrupted installation space check
			# failed and that code in overlaysav.sh should
			# not be done. We can't just set type to NEWINSTALL
			# because main routine would interpret this as
			# a return to the second boot floppy.
			INSTALL_TYPE=RENEWINSTALL
			return
			;;
        UNIX_V4)	[ ! "$UPGERR" ] && {
				INSTALL_TYPE=UPGRADE
				return	
			}
			;;
        UNIX_DESTINY)	[ ! "$UPGERR" ] && {
				INSTALL_TYPE=OVERLAY
				return	
			}
			;;
    esac

    [ "$UPDEBUG" = YES ] && goany

    export os_str	# set to the UNIX version in chkunixrel.sh 

    [ "$UPGERR" = NOSPACE ] && {

	#  not enough free space
	# set the vars. needed in setinst.2 

	[ -f $UPTMP/spc_msg ] && {

		IFS=$SPACE

		while read fs req avail
		do
		case $fs in
		   /)    ROOTREQ=$req; ROOTAVL=$avail;
			 export ROOTREQ ROOTAVL ;;
		   /var) VARREQ=$req;  VARAVL=$avail;
			 export  VARREQ VARAVL ;;
		   /usr) USRREQ=$req;  USRAVL=$avail;
			 export  USRREQ USRAVL ;;
		   *)    ;;
		esac
		done <$UPTMP/spc_msg

		rm -f $UPTMP/spc_msg
	}
    }

    menu_colors warn

    [ "$UPDEBUG" = YES ] && goany

    menu -f ${HD_MENUS}/setinst.2 -o $UPTMP/$$.resp  2>/dev/null
    . $UPTMP/$$.resp

    [ "$RETURN_VALUE" = 1 ] && {  # do not continue 
	    cd /;
    	    logmsg "up-n-over: $UPGERR - powerdown." POWERDOWN
    }

    return
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

Welcome_Select_Mode() {

   [ "$UPDEBUG" = YES ] && set -x
   unset RETURN_VALUE

   # can't write in $UPTMP - / may be unmounted

   [ "$UPDEBUG" = YES ] && goany "before select mode screen - $exist_os"
   menu_colors regular

   [ "$UPDEBUG" = YES ] && goany

   menu -r -f ${FD_MENUS}/chkinst.4 -o $UPTMP/$$.resp 2>/dev/null
   . $UPTMP/$$.resp
   INSTALL_MODE=CUSTOM 

   [ "$RETURN_VALUE" = 1 ] && { 

	# INSTALL_MODE=AUTOMATIC if 1st menu entry  selected
	INSTALL_MODE=AUTOMATIC
   }

   [ "$UPDEBUG" = YES ] && goany

   return
}

#main

. ${SCRIPTS}/common.sh
. ${SCRIPTS}/updebug.sh

[ "$UPDEBUG" = YES ] && set -x

# This step is done only if an active UNIX partition is detected
# and the user selected up-n-over

[ "$INSTALL_TYPE" = UPNOVER ] || exit 0

UPG_GLOBALS=/tmp/upg_globals
[ -f "$UPG_GLOBALS" ] && . $UPG_GLOBALS

SPACE=" "
TAB="	"

[ "$UPDEBUG" = YES ] && goany "Screen about to clear"

menu -c		#clear the select mode screen

Set_Install_Type

[ "$UPDEBUG" = YES ] && goany

echo "\nINSTALL_TYPE=\"$INSTALL_TYPE\"" >> $GLOBALS

[ "${INSTALL_TYPE}" = "INTERRUPTED" ] && {
	exit 0
}

[ "${INSTALL_TYPE}" = "RENEWINSTALL" ] && {
	# interrupted install failed space check. Re-install of
	# base was forced in Set_Install_Type. Set INSTALL_TYPE
	# back to NEWINSTALL and bail out, so that we skip
	# overlaysav.sh
	INSTALL_TYPE=NEWINSTALL
	echo "\nINSTALL_TYPE=\"$INSTALL_TYPE\"" >> $GLOBALS
	exit 0
}

Welcome_Select_Mode		#get mode - custom or auto

[ "$UPDEBUG" = YES ] && goany

echo "INSTALL_MODE=\"$INSTALL_MODE\"" >> $GLOBALS
echo "\nINSTALL_TYPE=\"$INSTALL_TYPE\"" >> $GLOBALS

#INSTALL_TYPE should be set to NEWINSTALL, or UPGRADE or OVERLAY

[ $INSTALL_TYPE = NEWINSTALL ] && {

	rm -f /etc/vfstab

	# we will go to the beginning of INSTALL
	# clean up /tmp before restarting, otherwise we run out of space on RAM

	rm  -rf $UPTMP

	# we have to skip initialize.sh and chkinsttyp.sh
	# therefore create this indicator file  $CANTUPGRADE

	> $CANTUPGRADE

	# some links and mkdir in inittab.sh should not be done again.
	>/tmp/done.links

	echo "\nCANTUPGRADE=\"$CANTUPGRADE\"" >> $GLOBALS
	Umount	#umount whatever is mounted
	exit 255		#goback to beginning to do destructive
}

# can't delete installation from here onwards. DELKEY is used by
# trap.sh to suppress the cancel option.

echo "\nINSTALL_TYPE=\"$INSTALL_TYPE\"" >> $GLOBALS
echo "DELKEY=OFF" >> $GLOBALS
echo "export DELKEY" >> $GLOBALS

UPGRADE_STORE=/var/sadm/upgrade

[ -d $HDROOT/$UPGRADE_STORE ] || mkdir -p  $HDROOT/$UPGRADE_STORE
echo "UPGRADE_STORE=\"$UPGRADE_STORE\"" >> $GLOBALS

rm -rf $UPTMP/$$.*

[ "$UPDEBUG" = YES ] && goany "End of setinsttyp.sh"
