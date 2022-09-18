#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/fdscripts/chkinsttyp.sh	1.16"
#ident	"$Header: $"

# Logic is such that you won't get to this point until
# you have a >60Mb partition on the hard disk.
# So, the logic becomes simpler -- after fdisk,
# we are a new install if a VTOC isn't on the disk,
# and are a candidate for Upnover if there is.
# return 2 if we can't upNover or user opts not to.
# return 1 to indicate that user has selected to attempt
# UpnOver (or interrupted installation).

Set_Install_Type ()
{
    [ "$UPDEBUG" = YES ] &&  set -x

    # test for VTOC. This won't work if a VTOC doesn't exist
    # (0s0 would work, but 0s1 won't if VTOC doesn't exist)

    partsize -s /dev/rdsk/c0t0d0s1 1>/dev/null 2>&1

    case $? in
	0)	# we have a VTOC in the UNIX partition!
		# check swap size to figure out if up-n-over will be offered
		# Check_Swap_Size returns 1 for swap slice large enough
		#			  2 if the swap slice is small

		[ "$UPDEBUG" = YES ] &&  goany

		Check_Swap_Size

		[ $? = 2 ] && return 2	  # do NEWINSTALL

		# offer destructive vs. non-destructive (up-n-over) choices

		unset RETURN_VALUE
		menu_colors warn

		[ "$UPDEBUG" = YES ] &&  goany

  	        menu -r -f ${FD_MENUS}/chkinst.3 -o /tmp/$$.resp 2>/dev/null
	        . /tmp/$$.resp

		# RETURN_VALUE=1 for up-n-over; 2 for NEWINSTALL
	    	return $RETURN_VALUE ;;

	*)	return 2;; # didn't work

      esac
}

# return 2 if the VTOC on the disk doesn't contain a swap slice
# big enough to run SVR4.2 on.

Check_Swap_Size ()
{
	# swap size must at least min ( 2*MEM, 13% of PART)
	# (memsize-based minimum adjusted for Version 4 defaults)

	# if swap slice is less than min, upgrade/overlay will not be offered

    	[ "$UPDEBUG" = YES ] && set -x

	# memsize returns bytes

	MIN=`memsize`
	#  add 1 Mb - 1 bytes to round up; memsize does not include shadow ram
	MIN=`expr ${MIN} + 1048575`
	MIN=`expr ${MIN} / 1048576`
	# only adjust memory requirement if <= 24 Mb. This matches Version 4
	if [ "${MIN}" -le 24 ]
	then
		if [ "${MIN}" -gt 12 ]
		then
			# scale by a factor of 1.5
			MIN=`expr ${MIN} \* 3 / 2` 
		else
			# scale by a factor of 2
			MIN=`expr ${MIN} \* 2` 
		fi
	fi

	# now see what minimum requirement for swap is as given
	# by 13% of the hard disk partition size

	# part size returns size of UNIX partition in Megabytes
	PART=`partsize`
	rc=$?
	# if partsize didn't work, we have problems. Error file is
	# "unexpected disk failure" -- unexpdisk
	[ "$rc" != "0" ] &&  error_out unexpdisk

	PART=`expr $PART \* 13 / 100`

	[ $MIN -gt $PART ] && MIN=$PART

	# Now check to see what we have allocated in the VTOC for swap
	# Definitely round up here...
	# slizesize returns blocks (512 bytes)

	SWAP=`slicesize /dev/rdsk/c0t0d0s0 2` # SWAP is always 2nd slice
	rc=$?
	# if slicesize didn't work, we have problems. Error file is
	# "unexpected disk failure" -- unexpdisk
	[ "$rc" != "0" ] &&  error_out unexpdisk

	SWAP=`expr $SWAP \* 512`
	SWAP=`expr $SWAP + 1048575` # round up
	SWAP=`expr ${SWAP} / 1048576` # convert to MBytes

	[ "$UPDEBUG" = YES ] &&  goany

	[ $SWAP -ge $MIN ] && return 1 	# check for destructive/non-destructive

    	[ "$UPDEBUG" = YES ] &&  goany "swap slice size is insufficient"

	# SWAP and MIN will be dispalyed in Mbytes.
	export MIN SWAP
	unset RETURN_VALUE
	menu_colors warn

	[ "$UPDEBUG" = YES ] &&  goany

	menu -f ${FD_MENUS}/chkinst.5 -o /tmp/$$.resp
	. /tmp/$$.resp

        [ ${RETURN_VALUE} = 1 ] && {  # do not continue 

      	    cd /;
            logmsg "Swap slice is too small. Do not to continue." POWERDOWN
   	}

	return 2 	# do NEWINSTALL
}

# Put up screen asking user to choose between AUTOMATIC and CUSTOM modes
Welcome_Select_Mode() {
	unset RETURN_VALUE
	menu_colors regular
	menu -f ${FD_MENUS}/chkinst.4 -o /tmp/$$.resp 2>/dev/null
	. /tmp/$$.resp
	INSTALL_MODE=CUSTOM
	[ "$RETURN_VALUE" = "1" ] && { 
		#INSTALL MODE AUTOMATIC if 1st menu entry selected
		INSTALL_MODE=AUTOMATIC;
	}
	return
}
#main
. ${SCRIPTS}/common.sh
. ${SCRIPTS}/updebug.sh

[ "$UPDEBUG" = YES ] &&  set -x

# $CANTUPGRADE is created if up-n-over can't be done 
# for whatever reason and this is the 2nd pass.

[ -f "$CANTUPGRADE" ] && exit 0

SPACE=" "
TAB="	"
TMP=$ROOT/tmp

INSTALL_TYPE=NEWINSTALL

# Set_Install_type returns  2 for NEWINSTALL, 1 for UPNOVER

[ "$UPDEBUG" = YES ] &&  goany

Set_Install_Type

[ "$?" = 1 ] && INSTALL_TYPE=UPNOVER

[ "$UPDEBUG" = YES ] && goany "After Set_Install_Type" 

echo "INSTALL_TYPE=\"$INSTALL_TYPE\"" >> $GLOBALS

[ "$INSTALL_TYPE" = UPNOVER ] && {

	echo "exit 0" >/tmp/disksetup.sh  #dummy step for UP_N_OVER
	rm -f /tmp/resp.$$

	[ "$UPDEBUG" = YES ] &&  goany

	exit 0
}

Welcome_Select_Mode

echo "INSTALL_MODE=\"$INSTALL_MODE\"" >> $GLOBALS
rm -f /tmp/resp.$$
exit 0
