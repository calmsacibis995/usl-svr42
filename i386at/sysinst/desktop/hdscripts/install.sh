#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/install.sh	1.2.1.40"

Add_Tape_To_Devtab () {

# $1 is the tape device name

echo 'Ntape1:'$1'n:::desc="Cartridge Tape Drive" volume="cartridge tape" type="ctape" scsi="true" removable="true" pdimkdtab="true" capacity="120000" bufsize="524288" norewind="'$1'n" erasecmd="/usr/lib/tape/tapecntl -e" display="true" rewindcmd="/usr/lib/tape/tapecntl -w"
ctape1:'$1':::desc="Cartridge Tape Drive" volume="cartridge tape" type="ctape" scsi="true" removable="true" pdimkdtab="true" capacity="120000" bufsize="524288" norewind="'$1'n" erasecmd="/usr/lib/tape/tapecntl -e" display="true"' >> /etc/device.tab

}

Prompt_To_Insert_Tape () {
# return 1 to indicate successful detect of tape
# return 2 to indicate return to previous menu
    [ -f ${TAPE_DRIVE} ] || {
	 return 2;
    } #return if TAPE_DRIVE file not readable
    >/tmp/tape_error
    MENU_TYPE=regular
    while [ 1 ]
    do
	unset RETURN_VALUE
	menu_colors ${MENU_TYPE}
        menu -f ${HD_MENUS}/install.3 -o /tmp/results 2>/dev/null
	. /tmp/results # set environment variables

	[ "${RETURN_VALUE}" = "2" ] && {
		return 2; # go back
	}

	menu_colors regular
        menu -r -f ${HD_MENUS}/install.2 -o /tmp/results 2>/dev/null

	echo EOTAPE >> ${TAPE_DRIVE}
	while read TAPE 
	do
    	   [ "${TAPE}" = "EOTAPE" ] && {
		echo EOTAPE > ${TAPE_DRIVE}n
		break;
	   }

	   # use no-rewind device on tapecntl to keep microcassette
	   # tape from popping out as a result of LOAD operation
	   # that occurrs during open on rewind device
	   tapecntl -t ${TAPE}n >/dev/null 2>&1 # retension tape cntrl
	   rc=$?

           [ $rc -eq 0 ] && {
		sleep 5	# wait for tape to reposition itself
		echo ${TAPE} > ${TAPE_DRIVE}n
		break;
		break
	   }
	done < ${TAPE_DRIVE}
	read TAPE < ${TAPE_DRIVE}n
        [ "${TAPE}" != "EOTAPE" ] && {
		Add_Tape_To_Devtab $TAPE
		echo "TAPE=${TAPE}" >> ${GLOBALS}
		break
	}
        MENU_TYPE=error
	cp ${HD_MENUS}/msg.err_2_tape /tmp/tape_error
	continue
    done
    unset MENU_TYPE
    sync
    return 1
}

Reclaim_Space() {
	rm -rf /FLOP_SEQ 1>/dev/null 2>&1
	rm -rf /INSTALL 1>/dev/null 2>&1
	rm -rf /LABEL 1>/dev/null 2>&1
	rm -rf /LABEL.4.0.dt 1>/dev/null 2>&1
	rm -rf /PKG_INFO 1>/dev/null 2>&1
	rm -rf /yes 1>/dev/null 2>&1
	rm -rf /etc/inst/locale/C/menus/fd 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/fdisk.sh 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/partitions.sh 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/custom_fs.sh 1>/dev/null 2>&1
	rm -rf /tmp/hdscripts.sh 1>/dev/null 2>&1
	rm -rf /tmp/disksetup.sh 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/chkinsttyp.sh 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/getFloppy3.sh 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/*list 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/*links 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/hdprepare.sh 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/mv2swap.sh 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/copy2hd.sh 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/chkunixrel.sh 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/setinsttyp.sh 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/overlaysav.sh 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts/overlaychk.sh 1>/dev/null 2>&1
	rm -rf /etc/disk.ele 1>/dev/null 2>&1 
	rm -rf /etc/disk.elesm 1>/dev/null 2>&1 
	rm -rf /etc/disk.elebig 1>/dev/null 2>&1
	# only need to uncompress once; and we already did it
	echo "exit 0" > /etc/inst/scripts/uncompress.sh
}

Install_Fnd_Set () {
   D=$1
   echo "$INSTALL_MODE $CONFIG_FLOPPY $CONF_RESP $CONF_RESP_READ $CONF_RESP_READ_LIST $CONF_RESP_CREATE $CONF_RESP_CREATE_LIST" >$INST_MODE
   # temporary -- make sure /tmp/response from menu call not around
   rm -f /tmp/response 2>/dev/null 1>/dev/null

   # save INSTALL_TYPE in /var/sadm/upgrade/install_type. chkpkgrel reads it.
   # chkpkgrel returns NEWINSTALL code for packages if INSTALL_TYPE=NEWINSTALL
   # we can't overlay, if INSTALL_TYPE=NEWINSTALL

   [ -f /var/sadm/upgrade/install_type ] || {
   	[ -d /var/sadm/upgrade ] || mkdir -p /var/sadm/upgrade
   	echo "INSTALL_TYPE=$INSTALL_TYPE" >/var/sadm/upgrade/install_type
   }

   QUIET=-q

   [ "$UPDEBUG" = "YES" ] && unset QUIET

   Reclaim_Space
   menu -c 2>/dev/null
   pkgadd -l -p $QUIET -d $D  $FND_SET
   pkgaddrc=$?

   # If it's a DESTRUCTIVE install, use the -i option to pkginfo to make
   # sure the pkgadd of the base was complete.  If it's a NON-DESTRUCTIVE
   # installation do NOT use the -i.  The -i could return an error if the
   # user had made some "unusual" changes in their system that caused an
   # error during the install.  If the user has done something "unusal"
   # to cause an error and we used the -i option, the 

   [ "$INSTALL_TYPE" = "NEWINSTALL" ] && PKGINFO_ARG=-i

   pkginfo $PKGINFO_ARG base > /dev/null 2>&1
   rc=$?

   [ "${rc}" = "0" ] && {
	return 1;
   }
   menu_colors error
   menu -f ${HD_MENUS}/install.4 -o /tmp/results 2>/dev/null
   return 2
}

Install_From_Floppy () {

   if [ "${MEDIA_CNT}" -gt 1 ]
   then
	unset RETURN_VALUE
	menu_colors regular
        menu -r -f ${HD_MENUS}/install.5 -o /tmp/results 2>/dev/null
	. /tmp/results
	rc=`expr $RETURN_VALUE`
   else
	menu_colors regular
        menu -r -f ${HD_MENUS}/install.6 -o /tmp/results 2>/dev/null
	rc=1
   fi
   [ "${rc}" = "2" ] && {
	return 2;
   }

   [ "${MEDIUM}" = "FLOP1" ] && {
	FLOP_DEV=/dev/dsk/${HAVEFLOP1}
   }
   [ "${MEDIUM}" = "FLOP2" ] && {
	FLOP_DEV=/dev/dsk/${HAVEFLOP2}
   }
   Install_Fnd_Set ${FLOP_DEV}
   return $?
}

Install_From_Tape () {
	sync
	D=${TAPE}
        [ "$STTY" ] && { stty $STTY ; }
        Install_Fnd_Set ${D}
	rc=$?
	return ${rc}
}

# main()

# do this before common.sh so that if interrupted install saves a
# higher val of PKG_STEP, we don't blow it away

. ${SCRIPTS}/common.sh
. /usr/sbin/pkginst/updebug

PKG_STEP=0

while [ "${PKG_STEP}" != "4" ]
do

[ "${PKG_STEP}" = "0" ] && {
	PKG_STEP=1
}

[ "${PKG_STEP}" = "1" ] && {

   CNT=0
   NF1=0
   NF2=0
   NC1=0
   export NC1 NF1 NF2
   [ -n "${HAVEFLOP1}" ] && {
      NF1=1
      CNT=`expr ${CNT} + 1`
      MEDIUM=FLOP1
   }
   [ -n "${HAVEFLOP2}" ] && {
      NF2=1
      CNT=`expr ${CNT} + 1`
      MEDIUM=FLOP2
   }
   [  -f $TAPE_DRIVE ] && {
      NC1=1
      CNT=`expr ${CNT} + 1`
      MEDIUM=CART_TAPE
   }
   MEDIA_CNT=${CNT}
   export MEDIA_CNT

   [ "${MEDIA_CNT}" = "0" ] && {
      menu_colors error
      menu -f ${HD_MENUS}/install.8 -o /tmp/results 2>/dev/null
      logmsg "Couldn't find tape or floppy drive to use as media device" POWERDOWN
      # WON'T REACH HERE
   }

   # only run the menu to prompt for media type if there is a choice
   [ "${MEDIA_CNT}" != "1" ] && {
	unset RETURN_VALUE
	menu_colors regular
	menu -f ${HD_MENUS}/install.7 -o /tmp/results 2>/dev/null
	. /tmp/results
	rc=`expr ${RETURN_VALUE}`
	# menu will return the index number selected from the menu (1-based)
	case ${MEDIA_CNT} in
	3) case ${rc} in
	   1) MEDIUM=FLOP1;;
	   2) MEDIUM=FLOP2;;
	   3) MEDIUM=CART_TAPE;;
	   esac;;
	2) [ -z "${HAVEFLOP1}" ] && {
		case ${rc} in
	   	1) MEDIUM=FLOP2;;
	   	2) MEDIUM=CART_TAPE;;
	   	esac
	   };
	   [ -z "${HAVEFLOP2}" ] && {
		case ${rc} in
	   	1) MEDIUM=FLOP1;;
	   	2) MEDIUM=CART_TAPE;;
	   	esac
	   }
	   [ -n "${HAVEFLOP1}" -a -n "${HAVEFLOP2}" ] && {
		case ${rc} in
	   	1) MEDIUM=FLOP1;;
	   	2) MEDIUM=FLOP2;;
	   	esac
	   };;
	esac
   }
   PKG_STEP=2
   echo "PKG_STEP=2" >> ${GLOBALS}
   echo "MEDIUM=${MEDIUM}" >> ${GLOBALS}
   echo "MEDIA_CNT=${MEDIA_CNT}" >> ${GLOBALS}
}

[ "${PKG_STEP}" = "2" ] && {
   
   rc=1
   if [ "${MEDIUM}" = CART_TAPE ]; then
	Prompt_To_Insert_Tape 
	rc=$?
   fi
   if [ "${rc}" = "1" ]
   then
   	PKG_STEP=3
   else
   	PKG_STEP=1
   fi
   echo "PKG_STEP=${PKG_STEP}" >> ${GLOBALS}
}

[ "${PKG_STEP}" = "3" ] && {
   if [ "$MEDIUM" = CART_TAPE ]; then
	Install_From_Tape
	rc=$?
   else
	Install_From_Floppy
	rc=$?
   fi
   if [ "${rc}" = "1" ]
   then
   	PKG_STEP=4
   else
   	PKG_STEP=1
   fi
   echo "PKG_STEP=${PKG_STEP}" >> ${GLOBALS}
}
sync
done
