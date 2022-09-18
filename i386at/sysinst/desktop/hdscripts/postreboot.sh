#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/postreboot.sh	1.26"
#ident	"$Header: $"

trap 'exit 3' 15
trap '' 2

logmsg()
{
	return
}

bye_bye ()
{
	menu -c >/dev/console
	echo "The system is coming up." > /dev/console
	
	rm -f /tmp/resp.$$
	
	sync
	sync

	rm -rf /etc/init.d/S02POSTINST 1>/dev/null 2>&1
	rm -rf /etc/rc2.d/S02POSTINST 1>/dev/null 2>&1

	[ "$UPDEBUG" = "YES" ] && exit 0
	
	# Clean up
	
	rm -f /sbin/fakevtoc /sbin/v_remount /sbin/ttyflushin /sbin/addswap 1>/dev/null 2>&1
	rm -f /sbin/removeswap /sbin/printvtoc /sbin/slicesize /sbin/listswap 1>/dev/null 2>&1
	rm -f /sbin/chroot /sbin/instcmd /sbin/setpasswd /sbin/stepper 1>/dev/null 2>&1
	
	rm -rf /FLOP_SEQ 1>/dev/null 2>&1
	rm -rf /INSTALL 1>/dev/null 2>&1
	rm -rf /LABEL 1>/dev/null 2>&1
	rm -rf /LABEL.4.0.dt 1>/dev/null 2>&1
	rm -rf /PKG_INFO 1>/dev/null 2>&1
	rm -rf /yes 1>/dev/null 2>&1
	rm -rf /etc/inst/locale/C/menus/fd 1>/dev/null 2>&1
	rm -rf /etc/inst/locale/C/menus/hd 1>/dev/null 2>&1
	rm -rf /etc/inst/scripts 1>/dev/null 2>&1
	rm -rf /etc/globals.sh 1>/dev/null 2>&1
	rm -rf /etc/disk.ele 1>/dev/null 2>&1
	rm -rf /etc/disk.elesm 1>/dev/null 2>&1
	rm -rf /etc/disk.elebig 1>/dev/null 2>&1
	rm -rf /etc/inst/.ovfstab /etc/inst/.odevice.tab 1>/dev/null 2>&1
	rm -rf /dev/.ordsk /dev/.odsk /dev/.ormt 1>/dev/null 2>&1
	rm -rf /usr/bin/bootcntl 1>/dev/null 2>&1
	rm -rf /etc/inst/up/straglers.v4 1>/dev/null 2>&1
	rm -rf /etc/inst/up/base.v4.log 1>/dev/null 2>&1
	rm -rf /etc/inst/up/boot.inst 1>/dev/null 2>&1
	rm -rf /etc/inst/up/up.err 1>/dev/null 2>&1
	rm -rf /var/sadm/upgrade/var/sadm/install/contents 1>/dev/null 2>&1

	exit 0
}

#
# This procedure is used during the reconfiguration of drivers
# and tunables for an UPGRADE installation.
#

convert_to_decimal ()
{
	#
	# This procedure takes two arguments, the number to convert and a
	# file to write the converted number into.  I could just set the
	# new value to a SHELL variable, but I don't want to depend on how
	# the shell deals with values being set within a shell procedure.
	#

	[ "$UPDEBUG" = "YES" ] && set -x

	[ $# != 2 ] && {

		# This should never happen, but it will help debugging.

		echo "Usage: $0 number file"
		return 1
	}

	NUM=$1
	FILE=$2

	# Verify the arguments are NOT NULL strings.

	[ ! "$NUM" -o ! "$FILE" ] && return 1

	#
	# Verify I can create the file.  The () are required otherwise
	# an error message will get blasted to the screen if the file
	# cannot be created.  Trying to redirect to /dev/null failed to
	# stop the error message.  Redirecting the error message from the
	# sub-shell works.
	#

	(> $FILE) 2>/dev/null

	[ $? != 0 ] && return 1

	BASE=10
	REGEX="\([0-9]*\)"

	for len in 2 1
	do
		#
		# If the string being checked for the substr is NULL
		# or less than $len characters, expr will return the
		# string and exit with 1.  For our purposes, we don't
		# need to be concerned about a failure from expr.
		#

		PREFIX=`expr substr "$NUM" 1 $len 2>/dev/null`

		[ "$PREFIX" = "0x" -o "$PREFIX" = "0X" ] && {

			#
			# "bc" is very strict.  It does NOT like 0x or 0X
			# if ibase=16 and it does not allow lower case
			# letters in a hex number.
			#

			NUM=`expr substr "$NUM" 3 16 2>/dev/null |
				sed -e "s/a/A/g" -e "s/b/B/g" -e "s/c/C/g" \
				    -e "s/d/D/g" -e "s/e/E/g" -e "s/f/F/g"`

			# We need to catch the NULL string again here.

			[ ! "$NUM" ] && return 1

			BASE=16
			REGEX="\([0-9A-F]*\)"
			break
		}

		[ "$PREFIX" = "0" ] && {

			BASE=8
			REGEX="\([0-7]*\)"
			break
		}
	done

	#
	# "bc" is very un-robust, I could NOT get it to exit with a
	# non-zero value even if I gave it a syntax error.  So lets
	# verify the string is "well formed" for the base we set.
	#

	CHK_NUM=`expr $NUM : "$REGEX" 2>/dev/null`

	[ "$CHK_NUM" != "$NUM" ] && return 1

	NEW_NUM=`bc <<- EOF
			ibase=$BASE
			$NUM
			EOF`

	echo $NEW_NUM >$FILE

	[ "$UPDEBUG" = "YES" ] && goany

	return 0
}

# main ()

######## initialize environment .....

SBINPKGINST=/usr/sbin/pkginst
SPACE=" "
TAB="	"
ETCINST=/etc/inst
UPGRADE_STORE=/var/sadm/upgrade
UPINSTALL=$ETCINST/up
UPERR=$UPINSTALL/up.err
CONF=/etc/conf
CONFv4=/etc/conf.v4
CONF_ORIG=/etc/CONF.ORIG

# put saved copy of globals.sh back in /tmp

cp /etc/globals.sh /tmp 1>/dev/null 2>/dev/null

# get boot floppy install environment -- via common.sh.
# however, get /etc/profile as well, just in case

. /etc/profile
. /etc/inst/scripts/common.sh
. $SBINPKGINST/updebug

[ "$UPDEBUG" = YES ] && set -x

PATH=:/usr/bin:/etc:/sbin:/usr/sbin:$PATH; export PATH

# initialize TERM to AT386-M (monochrome) if not defined

[ -z "${TERM}" ] && TERM=AT386-M
export TERM PATH

# work around for Dell VGA fast-write bug. Force text mode to color
# 80x25 if TERM is AT386

[ "${TERM}" = "AT386" ] && {
	stty VGA_C80x25 </dev/console 1>/dev/null 2>/dev/null
	echo "\033[0m\033[=0E\033[=7F\033[=1G\033[0m\033[J\033[7m\033[m" > /dev/console 2>&1
	echo "\033[2J\033[H" > /dev/console 2>&1 # CLEAR the SCREEN
}
RECONFIG_MARKER=/etc/inst/.kern_rebuild

[ -z "${LANG}" ] && LANG=C

MSE_MENUS=/etc/inst/locale/${LANG}/menus/hd
USER_MENUS=/etc/inst/locale/${LANG}/menus/hd
UP_MENUS=/etc/inst/locale/${LANG}/menus/hd

#
#  If no ${LANG} directory, fall back on the C-locale for
#  menu files.
#

if [ ! -d ${MSE_MENUS} ]
then
	MSE_MENUS=/etc/inst/locale/C/menus/mse
fi

if [ ! -d ${USER_MENUS} ]
then
	USER_MENUS=/etc/inst/locale/C/menus/hd
fi

if [ ! -d ${UP_MENUS} ]
then
	UP_MENUS=/etc/inst/locale/C/menus/hd
fi

if [ -d /etc/inst/locale/${LANG}/menus/upgrade ]
then
	UPGRADE_MSGS=/etc/inst/locale/${LANG}/menus/upgrade
else
	UPGRADE_MSGS=/etc/inst/locale/C/menus/upgrade
fi

#
#  Set up to use menu_colors; default to C-locale if ${LANG}'s dir has
#  no menu_colors.sh
#

if [ -f /etc/inst/locale/${LANG}/menus/menu_colors.sh ]
then
	. /etc/inst/locale/${LANG}/menus/menu_colors.sh
else
	. /etc/inst/locale/C/menus/menu_colors.sh
fi

[ "$UPDEBUG" = "YES" ] && goany

#
# The file $RECONFIG_MARKER is created by the boot floppy script reboot.sh.
# It is created if this is an UPGRADE installation AND there is something
# to "reconfigure" AND the user slected to reconfigure their system.
#

[ -f $RECONFIG_MARKER ] && {

	[ "$UPDEBUG" = "YES" ] && set +x && goany

	#
	# Delete the file we key off of because we don't want to hit
	# this section again.
	#

	rm -f $RECONFIG_MARKER

	unset RETURN_VALUE
	menu_colors regular
	menu -r -f $UPGRADE_MSGS/recon.working -o /dev/null >/dev/console

	[ "$UPDEBUG" = "YES" ] && set -x

	#
	# First we'll duplicate the /etc/conf tree resulting from the
	# installation of the Foundation Set.  We're doing this in case
	# the kernel will NOT build after reconfiguring the old drivers.
	#
	# This will make it very very easy to "unconfigure" the drivers
	# in case the kernel will not build when we're done.
	#

	mkdir $CONF_ORIG 2>/dev/null
	cd $CONF
	find . -print | cpio -pdl $CONF_ORIG >>$UPERR 2>&1

	#
	# Then we break the links of the files that we will modify directly
	# during the reconfiguration and make copies.
	#

	for i in cf.d/mtune		# ANY MORE ???
	do
		[ -f $CONF_ORIG/$i ] && {

			rm -f $CONF_ORIG/$i

			# use cpio -m to preserve own/mod/grp

			echo $i | cpio -pdmu $CONF_ORIG >/dev/null 2>&1
		}
	done

	[ "$UPDEBUG" = "YES" ] && goany

	# Now start the reconfiguration process

	cd $CONFv4
	rm -f $UPINSTALL/mod_failed
	mkdir work >>$UPERR 2>&1

	#
	# Now I need to do something repulsive-a special case kludge.
	# If I don't do this, the Version 4 'ip' driver will produce an
	# unresolved reference during the idbuild, because the variable
	# "ifstats" has moved from kernel/space.c in Version 4 to
	# ip/space.c in the the new system.  At this point, we have the
	# neither of these space.c's.
	#
	# NOTE: ifstats is declared as int in the Version 4 kernel/space.c
	#       and stuct *ifstats in the new ip/space.c. (remember we
	#       still have the old ip driver).
	#

	[ -f pack.d/ip/space.c ] &&
		echo "struct ifstats *ifstats;" >>pack.d/ip/space.c

	#
	# Make sure the directories I'm going to key off of exist and
	# are non-empty
	#

	CNT=0
	[ -d sdevice.d ] && CNT=`ls -1 sdevice.d | wc -l`
	[ $CNT != 0 ] && RECONFIG="sdevice.d/*"
	CNT=0
	[ -d sfsys.d ] && CNT=`ls -1 sfsys.d | wc -l`
	[ $CNT != 0 ] && RECONFIG="$RECONFIG sfsys.d/*"

	[ "$RECONFIG" ] &&
	for i in $RECONFIG
	do
		[ "$UPDEBUG" = "YES" ] && goany

		mod=`basename $i`
		dir=`dirname $i`

		$CONF/bin/idcheck -p $mod
		rc=$?

		[ $rc -ge 23 ] && {

			# The driver is already installed in the new system.

			rm -rf `find . -name $mod -print`

			continue
		}

		# We need to install the driver into current system.

		rm -rf work/*

		if [ "$dir" = "sdevice.d" ]
		then
			cp sdevice.d/$mod work/System
			grep "^$mod[ 	]" cf.d/mdevice >work/Master
		else
			cp sfsys.d/$mod work/Sfsys
			cp mfsys.d/$mod work/Mfsys
		fi

		[ -f init.d/$mod ] && cp init.d/$mod work/Init
		[ -f rc.d/$mod ] && cp rc.d/$mod work/Rc
		[ -f sd.d/$mod ] && cp sd.d/$mod work/Sd
		[ -f node.d/$mod ] && cp node.d/$mod work/Node
		cp pack.d/$mod/* work
		[ -f work/space.c ] && mv work/space.c work/Space.c
		[ -f work/stubs.c ] && mv work/stubs.c work/Stubs.c

		cd work

		[ "$UPDEBUG" = "YES" ] && goany

		# I hate special cases, but the RFS driver changed to rfs.

		[ "$mod" = "RFS" ] && $CONF/bin/idinstall -d rfs >/dev/null 2>&1

		#
		# The idinstall -d is being done for cautionary reasons.
		# If a driver is currently installed (e.g. 'ip' comes with
		# stubs.c) idinstall -a will fail.  I could check for just
		# a stubs.c file and only then execute it -d, but I'm not
		# sure if there are other cases I may need to handle.  I'd
		# rather be slow and paranoid than leave a potential hole.
		#

		$CONF/bin/idinstall -d $mod >/dev/null 2>&1
		$CONF/bin/idinstall -a -e $mod >>$UPERR 2>&1
		rc=$?

		cd ..

		[ "$rc" != "0" ] && {
		
			echo $mod >>$UPINSTALL/mod_failed
			rm -f work/*
			continue
		}

		# Now I need to move misc stuff from old pack.d to new pack.d

		mv work/* $CONF/pack.d/$mod >/dev/null 2>&1

		# Do I need to do anything about sync'ing contents file
		# for a potential pkgrm ??
	done

	[ "$UPDEBUG" = "YES" ] && goany

	#
	# Now I need to sync tunables.  The work to partition the Version 4
	# tunables into $UPINSTALL/tune.addem and $UPINSTALL/tune.exist was
	# done in the script reboot.sh on the boot floppies.
	#
	# First append the tunables we found to the current mtune file.  For
	# backwards compatibility, the current idtools still support a driver
	# package adding tunables to mtune, so by simply appending to the
	# the existing mtune, we're assured the tunables will get picked up.
	#
	# Another alternative was to create a new file(s) in mtune.d, but
	# there is a problem with that--we don't know what tunables belong
	# to what drivers, so we'd be forced to create a new file (e.g.
	# mtune.d/upgrade) that had NO corresponding driver.
	#
	# Appending them to the existing mtune file also eliminates a
	# potential problem when removing a driver package.  The package's
	# remove script # can remove the tunables as always.
	#
	# There is still a problem that needs to be dealt with.  I'll use
	# the inet package as an example.  We've just appended the Version
	# 4 inet tunables to the current mtune file.  Then we "upgrade" to
	# the new inet package.  The pkgadd of the new package results in
	# inet tunables being added as mtune.d/inet.  Now we have two sets
	# of potentially conflicting inet tunables.
	#
	# The solution was to enhance the idtools to support this and have
	# the values in mtune.d/* supercede the values in cf.d/mtune.  This
	# will work, but we may be left with obsolete tunables in cf.d/mtune.
	#

	[ -f $UPINSTALL/tune.addem ] &&
		cat $UPINSTALL/tune.addem >>$CONF/cf.d/mtune

	# Now we need to tweak the values of the current tunables.

	while read TOKEN CURRENTv4 MINv4 MAXv4
	do
		[ "$UPDEBUG" = "YES" ] && goany

		#
		# First we need to see if the Version 4 default has been
		# overriden by a value in the Version 4 stune file.
		#

		grep "^$TOKEN[ 	]" $CONFv4/cf.d/stune >/tmp/v4.$$

		[ $? = 0 ] && read JUNK CURRENTv4 </tmp/v4.$$

		# Then we'll get the current information for the new system

		$CONF/bin/idtune -g $TOKEN >/tmp/Dest.$$

		[ $? = 0 ] && read CURRENT DEFAULT MIN MAX JUNK </tmp/Dest.$$

		#
		# If the following string compare is successful, there is
		# nothing to do, so we can skip all the convert_to_decimal
		# calls below.  This will speed things considerably.
		#

		[ "$CURRENTv4" = "$CURRENT" ] && continue

		#
		# Now we have the following cases:
		#
		# - The Version 4 value is within the current MIN MAX limits
		# - The Version 4 value is greater than the current MAX
		# - The Version 4 value is less than the current MIN
		#
		# So the first thing we'll do is adjust to current MIN/MAX
		#
		# The values can be either decimal, octal or hex.  Since the
		# shell does NOT recognize hex or octal values, they need to
		# be converted to a common base before comparison.
		#
		# If there is any problem converting them to decimal, then
		# I will not try to "update" this particular tunable.
		#

		convert_to_decimal $CURRENTv4 /tmp/num.$$
		[ $? != 0 ] && continue
		read CONV_CURRv4 </tmp/num.$$

		convert_to_decimal $CURRENT /tmp/num.$$
		[ $? != 0 ] && continue
		read CONV_CURR </tmp/num.$$

		convert_to_decimal $MAX /tmp/num.$$
		[ $? != 0 ] && continue
		read CONV_MAX </tmp/num.$$

		convert_to_decimal $MIN /tmp/num.$$
		[ $? != 0 ] && continue
		read CONV_MIN </tmp/num.$$

		[ $CONV_CURRv4 -gt $CONV_MAX ] && {
		
			CONV_CURRv4=$CONV_MAX
			CURRENTv4=$MAX
		}

		[ $CONV_CURRv4 -lt $CONV_MIN ] && {
		
			CONV_CURRv4=$CONV_MIN
			CURRENTv4=$MIN
		}

		#
		# Now use idtune to merge the value.  I want to use the
		# "real" value here rather that the "converted" value.
		#

		[ $CONV_CURRv4 -gt $CONV_CURR ] &&
			$CONF/bin/idtune $TOKEN $CURRENTv4

	done <$UPINSTALL/tune.exist

	[ "$UPDEBUG" = "YES" ] && goany "About to remove /tmp files"

	rm -f /tmp/*.$$

	# Rebuild the kernel with the new modules.

	$CONF/bin/idbuild -B >$UPGRADE_STORE/idbuild.err 2>&1 || {

		[ "$UPDEBUG" = "YES" ] && goany "FAIL: idbuild"

		# The idbuild failed, first restore original /etc/conf tree.

		rm -rf $CONF
		mv $CONF_ORIG $CONF

		# set own/mod/grp correctly

		set `grep "^/etc/conf d " /var/sadm/install/contents`

		[ "$6" ] && {	# Just to be safe

			chmod $4 $CONF
			chown $5 $CONF
			chgrp $6 $CONF
		}

		BUILD_FAILED=YES

		#
		# Non-requirement: clean up /etc/conf.v4 so only relevant
		# stuff remains.  This includes cf.d/mdevice and
		# cf.d/[sm]fsys.
		#

		# TBD

		#
		# Requirements state to save everything in $UPGRADE_STORE/v4
		# if the kernel fails to build.
		#

		cd $CONFv4
		mkdir -p $UPGRADE_STORE/etc/conf >/dev/null 2>&1

		find * -print |
			cpio -pdlmu $UPGRADE_STORE/etc/conf >>$UPERR 2>&1

		menu_colors warn

		[ "$UPDEBUG" = "YES" ] && set +x && goany

		unset RETURN_VALUE
		menu -f $UPGRADE_MSGS/idbuild.fail -o /dev/null >/dev/console
	}

	[ "$UPDEBUG" = "YES" ] && goany && set -x

	[ "$BUILD_FAILED" != "YES" ] && {

		# I should deal with mod_failed here by altering message !!

		[ "$UPDEBUG" = "YES" ] && set +x && goany

		unset RETURN_VALUE
		menu_colors regular

		menu -f $UPGRADE_MSGS/reconfig.aok -o /dev/null >/dev/console

		[ "$UPDEBUG" = "YES" ] && set -x

		$CONF/bin/idcpunix >>$UPERR 2>&1

		# reboot system

		sync; sync; sync;
		uadmin 2 1
	}

}	# End of "Reconfiguratation of Drivers"

cd /
rm -rf $CONFv4 1>/dev/null 2>&1

[ "$UPDEBUG" = "YES" ] && goany

###### Begin mouse section....

# initialize default settings of mouse parameters. At this
# point in time, MOUSEBUTTONS is the only 1.

default_mouse_resp()
{
    MOUSEBUTTONS=2
}


# present menu for selecting mouse time. Run mouseadmin -t to
# test mouse changes. Return result of mouse test or 99 if no
# mouse selected.

Select_Mouse ()
{

while [ 1 ]
do
     unset RETURN_VALUE
     [ "${CONFIGED_MSE}" ] && {
     
         RETURN_VALUE=$CONFIGED_MSE
         export RETURN_VALUE
     }

     menu_colors regular
     menu -f $MSE_MENUS/chkmouse.1 -o /tmp/resp.$$ 2>/dev/null > /dev/console
     . /tmp/resp.$$
     ans=`expr ${RETURN_VALUE}`

     # deconfigure existing mouse
     mouseadmin 1>/dev/null 2>&1 <<EOF 
r
console
u
EOF

     case $ans in
         1)  Serial_Mouse_Port ;;#Serial mouse
         2)  Bus_Mouse_Interrupt_Vector; rc=$?; [ $rc = 0 ] && continue ;;
         3)  PS2_Mouse; rc=$?; [ $rc = 0 ] && continue ;;
         4)  unset MOUSEBUTTONS; return 99;; #No mouse
         *)  return 1;; #Invalid
     esac
     unset RETURN_VALUE
     menu_colors regular
     menu -f $MSE_MENUS/chkmouse.8 -o /dev/null 2>/dev/null > /dev/console
     mouseadmin -hidden -t < /dev/console 1>/dev/console 2>/dev/console
     rc=$?
     return $rc
done
}


# Called from Select_Mouse, set up PS/2 Mouse.

PS2_Mouse()
{
	# Make sure IVN 12 is available for use or is already
	# in use by m320 driver (that's us)
	IVNUSER=`/etc/conf/bin/idcheck -r -v 12`
	rc=$?
	if [ "$rc" != "0" -a "$IVNUSER" != "m320" ]
	then
		# IVN 12 in use by other driver. Give 'em choice of
		# shutdown or selecting another mouse type.
		rm -fr /tmp/ps2mse.$$ 2>/dev/null
		unset RETURN_VALUE
		MSE_TYP=PS2 export MSE_TYP
		menu_colors error
		menu -f $MSE_MENUS/chkmouse.5 -o /tmp/ps2mse.$$ 2>/dev/null > /dev/console
		. /tmp/ps2mse.$$
		if [ "$RETURN_VALUE" = "1" ]
		then
			return 0 # try a different mouse
		else
			# shut system down
			menu -c 1>/dev/console 2>&1
			sync; sync; sync;
			uadmin 2 0
		fi
	fi	
	# Ask use for # of mouse buttons
        unset RETURN_VALUE
	menu_colors regular
	menu -r -f $MSE_MENUS/chkmouse.4 -o /tmp/ps2mse.$$ 2>/dev/null > /dev/console

	# This sets MOUSEBUTTONS env variable
	. /tmp/ps2mse.$$
	/usr/bin/mouseadmin -a console m320 > /dev/null 2>&1
	return 1
}

# Called by Select_Mouse; ask for TTY port and # of buttons
Serial_Mouse_Port ()
{

     # Do we have second serial port?
     SPORT2=Yes
     /usr/sbin/check_devs -s /dev/tty01
     rc=$?
     [ "$rc" != 0 ]
	SPORT2=No
     export SPORT2

     # Ask for port and num. of buttons
     unset RETURN_VALUE
     menu_colors regular
     menu -f $MSE_MENUS/chkmouse.2  -o /tmp/smse.$$ 2>/dev/null > /dev/console

     . /tmp/smse.$$ # set environment variables SPORT and MOUSEBUTTONS
     if [ "${SPORT}" = "COM1" ]
     then
	/usr/bin/mouseadmin -a console tty00 1>/dev/null 2>&1
     else 
	/usr/bin/mouseadmin -a console tty01 1>/dev/null 2>&1
     fi
}

# Called from Select_Mouse. Get bus mouse interrupt vector and
# num mouse buttons
Bus_Mouse_Interrupt_Vector ()
{
     # see what interrupt vectors are available for the mouse
     # an IVN is OK if idcheck says it isn't in use or is in
     # use by "bmse" -- that's us.

     export MSEINT
     CNT=0

     # strategy is to set CNT to # of IVNs, write each IVN available
     # to file with IVNs tab-separated, so can be read into
     # shell variables later.

     IVNUSER=`/etc/conf/bin/idcheck -r -v 9 2>/dev/null`
     rc=$?
     > /tmp/bmse.intr
     if [ "$rc" = 0 -o "${IVNUSER}" = "bmse" ]
     then
	echo -n "2${TAB}" >> /tmp/bmse.intr
	CNT=`expr ${CNT} + 1`
     fi
     IVNUSER=`/etc/conf/bin/idcheck -r -v 3 2>/dev/null`
     rc=$?
     if [ "$rc" = 0 -o "${IVNUSER}" = "bmse" ]
     then
	echo -n "3${TAB}" >> /tmp/bmse.intr
	CNT=`expr ${CNT} + 1`
     fi
     IVNUSER=`/etc/conf/bin/idcheck -r -v 4 2>/dev/null`
     rc=$?
     if [ "$rc" = 0 -o "${IVNUSER}" = "bmse" ]
     then
	echo -n "4${TAB}" >> /tmp/bmse.intr
	CNT=`expr ${CNT} + 1`
     fi
     IVNUSER=`/etc/conf/bin/idcheck -r -v 5 2>/dev/null`
     rc=$?
     if [ "$rc" = 0 -o  "${IVNUSER}" = "bmse" ]
     then
	echo -n "5${TAB}" >> /tmp/bmse.intr
	CNT=`expr ${CNT} + 1`
     fi

     # read IVN values in bmse.intr into IVN1-IVN4.
     OIFS=${IFS}
     IFS=${TAB}
     read IVN1 IVN2 IVN3 IVN4 < /tmp/bmse.intr
     IFS=${OIFS}
     export CNT IVN1 IVN2 IVN3 IVN4
     [ "$CNT" = "0" ] && {

	# No interrupt vectors available. Must select a mouse type
	# other than bus mouse. Give user choice of shutdown or
	# choosing a different type.
        unset RETURN_VALUE
	MSE_TYP=BUS export MSE_TYP
	menu_colors error
	menu -f $MSE_MENUS/chkmouse.5 -o /tmp/bmse.$$ 2>/dev/null > /dev/console
	. /tmp/bmse.$$
     	rm -fr /tmp/bmse* 1>/dev/null 2>&1
	if [ "$RETURN_VALUE" = "1" ]
	then
		return 0
	else
		menu -c > /dev/console
		sync; sync; sync;
		uadmin 2 0
	fi
     }
     unset RETURN_VALUE
     menu_colors regular
     menu -r -f $MSE_MENUS/chkmouse.3  -o /tmp/bmse.$$ 2>/dev/null > /dev/console

     . /tmp/bmse.$$		# pick up shell var MSEINT from form
     rm -f /tmp/bmse.$$ 1>/dev/null 2>&1
     /usr/bin/mouseadmin -i $MSEINT -a console bmse > /dev/null 2>&1 
     return 1
}

what_mouse()
{
	[ "$UPDEBUG" = "YES" ] && set -x

    # find configured mouse info for upgrade/overlay
    # the configured mse is set as default in the env. var CONFIGED_MSE

    SD=/etc/conf/sdevice.d

    #
    # First check for bmse in $UPGRADE_STORE, if it exists, use it,
    # otherwise check for it in /etc/conf/sdevice.d.
    #
    # After we've been through here once, we NEVER want to use the values
    # in $UPGRADE_STORE/bmse again, so we need to rm the file.
    #

    BMSE=$UPINSTALL/bmse
    [ -f $BMSE ] || BMSE=/etc/conf/sdevice.d/bmse

    OIFS=${IFS}
    IFS=$TAB
    read xx MSE <$MSETAB
    IFS=${OIFS}
    
    case $MSE in
	bmse)
	    [ -f $BMSE ] || {
    		    logmsg "bmse in /usr/lib/mousetab, but $SD/bmse missing"
    		    return
	    }

		grep "^bmse" $BMSE >/tmp/bmse 2>/dev/null
		rm -f $UPGRADE_STORE/bmse

	    [ -f /tmp/bmse ] || {
    		    logmsg "$SD/bmse corrupted"
    		    return
	    }

	    read Dev Conf Unit Ipl Type MSEINT SIOA EIOA SCMA ECMA </tmp/bmse

    	    [ "$Conf" = N ] && {
    		    logmsg "bmse not configured in $SD/bmse"
    		    return
    	    }

	    [ -z "$MSEINT" ] || {
    	        [ "$MSEINT" = 9 ] && MSEINT=2	# postinstall script will
						# change it to 9
	        CONFIGED_MSE=2; export CONFIGED_MSE
	    }
    	    ;;
	m320)	# PS/2 mouse
	    CONFIGED_MSE=3; export CONFIGED_MSE
    	    ;;
	tty00|tty01)	
	    CONFIGED_MSE=1; export CONFIGED_MSE
	    export SPORT
    	    SMS_PORT=0
    	    [ $MSE = tty01 ] && {
        	SPORT=COM2;
    		SMS_PORT=1
    	    }
	    export MSE
    	    ;;
    esac

    [ -f /etc/default/mouse ] && {
    	OIFS=${IFS}
    	IFS="="
	# look for an entry setting MOUSEBUTTONS in /etc/default/mouse
	grep MOUSEBUTTONS /etc/default/mouse 2>/dev/null | read xx MOUSEBUTTONS
    	IFS=${OIFS}
    }
    export MOUSEBUTTONS
}


ps -ef > /tmp/foo.mse
MGRPID=`grep mousemgr /tmp/foo.mse`
rm -f /tmp/foo.mse 1>/dev/null 2>&1
if [ -z "$MGRPID" ]
then
	MGRPRESENT=No
	/usr/lib/mousemgr 1>/dev/null 2>/dev/null & # Start mouse mgr in background
else
	MGRPRESENT=Yes
fi

# fill in the default mouse response

while [ 1 ]
do
	default_mouse_resp

	MSETAB=/usr/lib/mousetab	
	OIFS=$IFS
	[ -f $MSETAB ] && what_mouse

	[ "$UPDEBUG" = YES ] && goany

	IFS=$OIFS
	Select_Mouse 
	rc=$?
	[ "${rc}" = "0" ] && {
     		unset RETURN_VALUE
     		menu_colors regular
     		menu -r -f $MSE_MENUS/chkmouse.6 -o /tmp/resp.$$ 2>/dev/null > /dev/console
		# Allow "-r" screen to be displayed for a couple seconds
		sleep 2
		break
	}
	[ "${rc}" = "99" ] && {
		break
	}
	unset RETURN_VALUE
     	menu_colors warn
     	menu -r -f $MSE_MENUS/chkmouse.7 -o /tmp/resp.$$ 2>/dev/null > /dev/console
     	. /tmp/resp.$$
     	ans=`expr ${RETURN_VALUE}`
	[ "$ans" = 2 ] && {
		menu -c > /dev/console
		sync; sync; sync;
		uadmin 2 0
	}
done

# save user response re: number of mouse buttons
[ -f /etc/default/mouse ] && {
	cat /etc/default/mouse | grep -v MOUSEBUTTONS > /tmp/mouse.$$
}
[ -n "${MOUSEBUTTONS}" ] && {
echo "MOUSEBUTTONS=${MOUSEBUTTONS}" >> /tmp/mouse.$$
mv /tmp/mouse.$$ /etc/default/mouse 1>/dev/null 2>&1
}
if [ "${MGRPRESENT}" = "No" ]
then
	# We started mousemgr above and need to kill it
	# so that when a later script starts it, it works
	ps -ef > /tmp/foo.mse
	MGRPID=`grep mousemgr /tmp/foo.mse`
	rm -rf /tmp/foo.mse 1>/dev/null 2>/dev/null
	set -a $MGRPID # $2 will now be PID of mousemgr
	exec 2>/dev/null
	kill -9 $2
	exec 2>/dev/console
fi


### Begin addusers section -- prompt for user acct info, root, user passwd

# Add a login ID ($1) to the packaging tool admin file specified by $2
# This is so that the owner account created here received mail from
# the packaging tools.

Add_Owner_To_Pkg() {

	MAILID=$1
	FILE=$2
	# Bail out if arguments not valid...
	[ ! -f ${FILE} ] && return
	[ "${MAILID}" = "" ] && return
	[ "${MAILID}" = "root" ] && return # root always configured

	# look for construct mail=<list of logins separated by spaces>
	# And then look for user ID $MAILID within that list
	# Either the pattern " <ID> " is in $2 or the pattern
	# " <ID>$" (where $ is end of line) is in the file.
	GREP1=`/sbin/grep "^mail=" ${FILE} | /sbin/grep " ${MAILID} "`
	GREP2=`/sbin/grep "^mail=" ${FILE} | /sbin/grep " ${MAILID}$"`
	if [ "${GREP1}" = "" -a "${GREP2}" = "" ]
	then
		# add the user to the list
		FNAME=/tmp/.mailtoid$$
		/usr/bin/sed \/\^mail\/s/\$\/" ${MAILID}"\/ < ${FILE} > ${FNAME}
		# Use cp to maintain perms on $FILE
		/sbin/cp ${FNAME} ${FILE}
		/sbin/rm -f ${FNAME} 1>/dev/null 2>&1
	fi
	# otherwise user was already in the list
}


Get_Root_Passwd ()
{
     	unset RETURN_VALUE
	menu_colors regular
	menu -f $USER_MENUS/addusers.1 -o /dev/null 2>/dev/null > /dev/console

	#
	#  In all cases, reset this user's password and get a new one.
	#  Also, check to make sure passwd executed correctly and reinvoke
	#  if necessary.
	#
	passwd -d root
	DONE=0

	while [ "${DONE}" = "0" ]
	do
		setpasswd root
		if [ $? = 0 ]
		then
			DONE=1
		fi
	done

}

Get_User_Passwd ()
{
	LOGIN=$1
	menu_colors regular
     	unset RETURN_VALUE
	menu -f $USER_MENUS/addusers.10 -o /dev/null 2>/dev/null > /dev/console

	#
	#  In all cases, reset this user's password and get a new one.
	#  Also, check to make sure passwd executed correctly and reinvoke
	#  if necessary.
	#
	passwd -d ${LOGIN}
	DONE=0

	while [ "${DONE}" = "0" ]
	do
		setpasswd ${LOGIN}
		if [ $? = 0 ]
		then
			DONE=1
		fi
	done

}

# addusers main()


[ "$UPDEBUG" = "YES" ] && set -x

> /tmp/err.login 

USERNUM=101	 # default to 101 for destructive installation
export USERNUM
DESKTOP_PRESENT=No
pkginfo desktop 1>/dev/null 2>/dev/null
[ "$?" = "0" ] && DESKTOP_PRESENT=Yes
export DESKTOP_PRESENT USERID

#
# If they choose to not merge their system files, then the /etc/passwd
# file we just installed will contain NO logins, so we're forced to
# set up a user account and we're also going to require a root passwd.
#
# If they choose AUTOMERGE=Yes, then we need to do something special for
# an UPGRADE.
#

[ "$AUTOMERGE" = "Yes" ] && {

	[ "$UPDEBUG" = "YES" ] && goany

	# If it's an overlay, they're already set up.

	[ "$INSTALL_TYPE" = "OVERLAY" ] && bye_bye

	[ "$UPDEBUG" = "YES" ] && goany

	#
	# If the desktop metaphor package is not installed, we don't
	# need to identify anyone as the owner 
	#

	[ "$DESKTOP_PRESENT" = "No" ] && bye_bye

	#
	# If we're doing an upgrade installation, we're going to let
	# them chose an existing login as the owner.
	#
	# On a v4 box. id 101 belongs to "install", and the Destiny passwd
	# file has logins "nobody" and "noaccess" that are not really valid
	# owners, so they will be removed from the list.
	#
	# For some reason, the awk was adding an extra space, so it only
	# pads with 6, while the echo uses 7.
	#

	cat /etc/passwd | awk -F: '$3 > 100 {print "      ", $1}' |
		egrep -v " install$| nobody$| noaccess$| vmsys$| oasys$" \
			>/tmp/logins.list

	[ -s /tmp/logins.list ] && {

		echo "       new_user" >>/tmp/logins.list

		#
		# This is needed as a place holder to make the menu tool
		# happy.  It an error occurs, we'll put something useful
		# in it to help the user understand the error of their way.
		#

		> /tmp/up_users.err

		while true
		do
			OWNER=new_user; export OWNER

			[ "$UPDEBUG" = "YES" ] && goany && set +x

			menu_colors regular
			unset RETURN_VALUE
			menu -f $USER_MENUS/addusers.6 -o /tmp/resp.$$ 2>/dev/null >/dev/console

			[ "$UPDEBUG" = "YES" ] && set -x

			. /tmp/resp.$$

			grep " $OWNER\$" /tmp/logins.list >/dev/null

			[ $? = 0 ] && break

			echo "\"$OWNER\" is not a valid choice, try again !" \
						>/tmp/up_users.err
		done

		[ $OWNER != new_user ] && {

			OIFS=${IFS}
			IFS=:
			set `grep "^$OWNER:" /etc/passwd`
			IFS=${OIFS}

			#
			# The following assignments will be defaults in
			# the addusers.3 form below.
			#

			USERNAME=$5; export USERNAME
			USERID=$1; export USERID
			USERNUM=$3; export USERNUM

			# This will be used to vary the behavior below

			UPGRADE=YES
		}
	}

	[ "$UPDEBUG" = "YES" ] && goany
}

# For either auto or custom installs, user created as desktop owner 
# if desktop software installed.

DESKTOP=MOTIF; export DESKTOP	# Default to Motif desktop environment

ADDUSERS=$USER_MENUS/addusers.3	# Regular user login request screen

#
# If we're UPGRADE'ing and AUTOMERGE=Yes and they selected an owner
# from the list of existing accounts, the only fields we need info
# for are Desktop Mode, so that's all we'll ask for.
#

[ "$UPGRADE" = "YES" -a "$DESKTOP_PRESENT" = "Yes" ] &&
	ADDUSERS=$USER_MENUS/addusers.4

MENU_TYPE=regular

while [ 1 ]
do
	[ "$UPDEBUG" = "YES" ] && goany && set +x

	menu_colors ${MENU_TYPE}
	unset RETURN_VALUE
	menu -f $ADDUSERS -o /tmp/resp.$$ 2>/dev/null > /dev/console

	[ "$UPDEBUG" = "YES" ] && set -x

	. /tmp/resp.$$		# add form responses to environment

	#
	# We need to do some error checking if they chose AUTOMERGE=No
	# or they chose AUTOMERGE=Yes and then selected "new_user" as the
	# owner of their system (see addusers.6 above).
	#

	[ "$AUTOMERGE" != "NULL" -a "$UPGRADE" != "YES" ] && {

		PASSWD=$UPGRADE_STORE/etc/passwd

		[ "$AUTOMERGE" = "Yes" ] && PASSWD=/etc/passwd

		# 
		# Check the old /etc/passwd file for the information they
		# specified.  There are 3 cases:
		#
		# 1-They specified a login and userid that match an entry
		#   in the old /etc/passwd.  In this case, a HOME directory
		#   already exists, so we'll change the command line args
		#   to useradd below to reflect this.
		#
		# ?? What if it's an OVERLAY and they've already set a
		#    desktop and/or owner ??
		# OR they now specify "None, do I unconfig them ??
		#    or do I warn them ??
		# OR they change from OL to Motif ??
		#
		#
		# 2-They specified a new login and new userid.	In this
		#   case, we continue as usual.
		#
		# 3-The last case is an error condition.  Either the login
		#   or userid match an entry in the old /etc/passwd, OR
		#   both match, but they don't correspond to the same
		#   person.  In this case, we present them with an error
		#   screen and make them try again.
		#

		OIFS=${IFS}
		IFS=:
		unset UID1 UNUM1 HOME UID2 UNUM2
		set `grep "^$USERID:" $PASSWD` >/dev/null
		[ $? = 0 ] && { UID1=$1; UNUM1=$3; HOME=$6; }
		set `grep "^[^:]*:[^:]*:$USERNUM:" $PASSWD` >/dev/null
		[ $? = 0 ] && { UID2=$1; UNUM2=$3; }
		IFS=${OIFS}

		if [ "$USERID" = "$UID1" -a "$USERNUM" = "$UNUM1" ] # CASE #1
		then
			HOMEARG=$HOME

			#
			# If we hit this case and AUTOMERGE=Yes, then they're
			# trying to pull a fast one on us.  They specified
			# "new_user" when asked to select an owner and then
			# proceeded to give an existing login/userid anyway.
			#

			[ "$AUTOMERGE" = "Yes" ] && UPGRADE=YES

		elif [ "$UID1" -o "$UNUM2" ]	# CASE #3, an ERROR !!!
		then
			# login exists with different userid

			ERRSCREEN=ERROR1
			ERRUID=$UID1
			ERRUNUM=$UNUM1

			# userid exists with different login

			[ "$UNUM2" -a -z "$UID1" ] && {
			
				ERRSCREEN=ERROR2
				ERRUID=$UID2
				ERRUNUM=$UNUM2
			}

			# both exist but not for same user

			[ "$UNUM2" -a "$UID1" ] && ERRSCREEN=ERROR3

			export ERRSCREEN ERRUID ERRUNUM

			[ "$UPDEBUG" = "YES" ] && goany && set +x

			unset RETURN_VALUE
			menu_colors warn
			menu -f $USER_MENUS/addusers.7 -o /dev/null 2>/dev/null >/dev/console
			menu_colors regular

			#
			# I could unset USERID & USERNUM, but I'm going to
			# leave them set, so they'll repopulate addusers.4
			#

			continue
		fi
	}

	[ "$UPDEBUG" = "YES" ] && goany && set -x

	#
	# When the following section is hit, the form comes back up with
	# the fields filled in with the same values the user just entered
	# except we reset the USERNUM to "".
	#
	# It would also be nice to have the current field be the USERNUM
	# field, BUT that may be asking for too much.
	#

	# What if they correctly specify a login and userid from the
	# old /etc/passwd, but the userid < 101 ???

	[ -d /var ] || mkdir /var
	HOMEDIR=/var
	[ -d /home ] && HOMEDIR=/home
	[ -z "$HOMEARG" ] && HOMEARG=$HOMEDIR/$USERID

	#
	# If all we're really doing is defining an owner for the case
	# of an upgrade and /etc/passwd has been updated with existing
	# accounts, then we don't need to add $USERID as a user here.
	#

	[ "$UPGRADE" != "YES" ] && {
	
		OIFS=${IFS}
		IFS=$TAB

		/usr/sbin/useradd -u $USERNUM -c "$USERNAME" \
			-d $HOMEARG -m $USERID 

		IFS=${OIFS}
		[ $? -ne 0 ] && {

			cp $USER_MENUS/err_user_login /tmp/err.login
			MENU_TYPE=warn
			continue
		}

		Get_User_Passwd $USERID
	}

	[ "$UPDEBUG" = "YES" ] && goany && set -x

	# What if you're set up as a user, and then remove the package ??
	# This is a general question, NOT up_n_over related.

	[ $DESKTOP_PRESENT = Yes ] && {

		# Is there a problem if UPGRADE=YES ?  Answer: NO !

		[ "$DESKTOP" != "NONE" ] && {

			# just to be safe, and prevent warnings

			[ "$INSTALL_TYPE" = "OVERLAY" ] &&
				/usr/X/adm/dtdeluser $USERID >/dev/null 2>&1

			#
			# If the DESKTOP chosen is MOTIF then we need
			# to use the -m option to dtadduser
			#
		
			[ "$DESKTOP" = "MOTIF" ] && MARG=-m
			/usr/X/adm/dtadduser $MARG $USERID
		}

		/usr/X/adm/make-owner $USERID
	}

	# Update "mail=root" entry in the package tools
	# administration files so that mail is sent to
	# the login ID just specified.

	for tfile in /var/sadm/install/admin/*
	do
		Add_Owner_To_Pkg $USERID $tfile
	done

	[ "$AUTOMERGE" != "Yes" ] && Get_Root_Passwd

	break
done

unset RETURN_VALUE
menu -f $USER_MENUS/addusers.8 -o /dev/null 2>/dev/null > /dev/console

# make sure /etc/vfstab has 644 permissions
chmod 644 /etc/vfstab
[ "${TERM}" = "AT386" ] && {
	# put colors back to normal
	stty VGA_C80x25 </dev/console 1>/dev/null 2>/dev/null
	echo "\033[0m\033[=0E\033[=7F\033[=0G\033[0m\033[J\033[7m\033[m" > /dev/console 2>&1
	echo "\033[2J\033[H" >/dev/console 2>&1 # CLEAR the SCREEN
}
bye_bye
