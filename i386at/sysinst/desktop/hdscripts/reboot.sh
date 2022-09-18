#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/reboot.sh	1.3.2.35"
#ident	"$Header: $"

# main()

sync
sync

. ${SCRIPTS}/common.sh

# copy postreboot.sh to init.d/rc2.d so that the script
# is run upon reboot. This script will configure V4 drivers
# for upgrade, config the mouse, ask about the user account,
# and clean up after the install scripts.

[ "$UPDEBUG" = "YES" ] && goany

cp ${SCRIPTS}/postreboot.sh /etc/init.d/S02POSTINST
ln /etc/init.d/S02POSTINST /etc/rc2.d/S02POSTINST 1>/dev/null 2>&1

# save globals file in /etc so it doesn't get deleted at reboot
cp ${GLOBALS} /etc/globals.sh 1>/dev/null 2>&1

rm -f /tmp/.last_step 1>/dev/null 2>&1

############### Begin UPGRADE AND OVERLAY #################

CONF=/etc/conf
ETCINST=/etc/inst
UPINSTALL=$ETCINST/up
SBINPKGINST=/usr/sbin/pkginst
UPGRADE_STORE=/var/sadm/upgrade
UPGRADE_MSGS=/etc/inst/locale/${LANG}/menus/upgrade
RECONFIG_MARKER=/etc/inst/.kern_rebuild

. $SBINPKGINST/updebug

rm -f /tmp/pkgfail 1>/dev/null 2>&1

#
#  
#  Use the menu tool to show what packages installed correctly and
#  what packages did not.
#


cp ${HD_MENUS}/reboot.2 /tmp/set.6.$$ > /dev/null 2>&1

while read PKGABBR PKGNAME
do
	pkginfo -i ${PKGABBR} > /dev/null 2>&1
	if [ $? = 0 ]
	then
		#
		#  Successful install.  Tack on to .top of /tmp/set.6
		#
		echo "\tSucceeded\t${PKGNAME}" >> /tmp/set.6.$$
	else
		#
		#  Unsuccessful install.  Tack on to .top of /tmp/set.6
		#  and touch pkgfail to let menu know that pkgs have failed.
		#
		> /tmp/pkgfail
		echo "\tFailed\t\t${PKGNAME}" >> /tmp/set.6.$$
	fi
done < /tmp/name_final_y

[ "$UPDEBUG" = "YES" ] && goany
#
# Use col -x to convert TABS to spaces to keep menu tool happy.
#
col -x < /tmp/set.6.$$ > /tmp/set.6 2>&1

############### Begin UPGRADE/OVERLAY ############

[ -f $UPGRADE_STORE/nomrg.list ] && {
	# NOMRGLIST is the list of pkgs whose system files are not combined.
	read NOMRGLIST <$UPGRADE_STORE/nomrg.list
	rm -f $UPGRADE_STORE/nomrg.list

	# NOMRGMSG is the name of the file containing the 'nomerge msg'
	NOMRGMSG=$UPGRADE_MSGS/mergefiles.4

	export NOMRGLIST NOMRGMSG
}

[ -f $UPGRADE_STORE/mrgfail.list ] && {
	# MRGFAILLIST is the list of pkgs for which 'up_merge' failed
	read MRGFAILLIST <$UPGRADE_STORE/mrgfail.list
	rm -f $UPGRADE_STORE/mrgfail.list

	#MRGFAILMSG is the name of the file containing 'up_merge failed' msg
	MRGFAILMSG=$UPGRADE_MSGS/mergefiles.2

	# MRGFAILFILES is the list of specific system files for various 
	# pkgs for which  up_merge failed

	MRGFAILFILES=$UPGRADE_STORE/mrgfail.files
	export MRGFAILLIST MRGFAILMSG MRGFAILFILES
}

############### End UPGRADE/OVERLAY ############

#
#  Put the finishing touches on this menu
#
[ -f /tmp/pkgfail ] && echo  >> /tmp/set.6
[ -f /tmp/pkgfail ] && echo "Details on any failures are detailed in electronic mail sent to the" >> /tmp/set.6
[ -f /tmp/pkgfail ] && echo "login ID \"root\"" >> /tmp/set.6
[ "${NOMRGLIST}" -o "${MRGFAILLIST}" ] && echo " " >> /tmp/set.6
[ "${NOMRGLIST}" -o "${MRGFAILLIST}" ] && echo "Press the 'F1' key for information about your System Setup Files." >> /tmp/set.6
echo  >> /tmp/set.6

[ -s /tmp/name_final_y ] && {
	menu_colors regular
	menu -f /tmp/set.6 -o /tmp/out 2>/dev/null
}

rm -f $UPGRADE_STORE/mrgfail.files

setup_reconfig ()
{
	menu_colors regular
	unset RETURN_VALUE

	menu -f $UPGRADE_MSGS/reconfig.sel -o /tmp/recon.$$

	[ "$UPDEBUG" = "YES" ] && set -x

	. /tmp/recon.$$

	[ "$RETURN_VALUE" = 2 ] && {

		#
		# The requirements state to save everything in $UPGRADE_STORE
		# if the user selects to NOT reconfigure their modules.
		#
		# The /etc/conf.v4 tree will get blown away in the rc script
		# postreboot.sh.
		#

		cd /etc/conf.v4
		mkdir -p $UPGRADE_STORE/etc/conf >/dev/null 2>&1

		find * -print |
			cpio -pdlmu $UPGRADE_STORE/etc/conf >>$UPERR 2>&1

		return
	}

	#
	# Now I'll create a file that the rc script will key off of
	# to decide whether or not to reconfigure the drivers.
	#

	> ${RECONFIG_MARKER}

	[ "$UPDEBUG" = "YES" ] && goany
}

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

	# Verify the number to convert is NOT a NULL string.

	[ ! "$NUM" ] && return 1

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

check_reconfig ()
{
	[ "$UPDEBUG" = "YES" ] && set -x

	#
	# The first thing we need to do is decide if we should even offer
	# them the option to reconfigure the Version 4 drivers and tunables
	# into the current system.  We will offer them the option if any
	# of the following three conditions are met:
	#
	# - There are drivers to reconfigure (da!)
	# - There are existing tunable values that require tweaking.
	# - There are NEW tunables to add.
	#

	rm -f /tmp/*.$$
	rm -f $UPINSTALL/tune.exist
	rm -f $UPINSTALL/tune.addem

	#
	# Partition the tunables from /etc/conf.v4/cf.d/mtune into two
	# files.  Those that exist in the new system and those that do not.
	#

	cat /etc/conf.v4/cf.d/mtune |
		grep -v "^[*#]" |
		grep -v "^[ 	]*$" >/tmp/mtune.$$

	while read TOK DFLT MIN MAX
	do
		# Check if the tunable exists in the current system

		$CONF/bin/idtune -g $TOK >/dev/null 2>&1

		if [ $? = 0 ]
		then
			echo "$TOK\t$DFLT\t$MIN\t$MAX" >>$UPINSTALL/tune.exist
		else
			echo "$TOK\t$DFLT\t$MIN\t$MAX" >>$UPINSTALL/tune.addem
		fi

	done </tmp/mtune.$$

	[ "$UPDEBUG" = "YES" ] && goany

	[ -f $UPINSTALL/tune.addem ] && {
	
		setup_reconfig
		return
	}

	#
	# If we're still here, check if there are any driver modules to
	# reconfigure.  Even though this is a simpler test than partitioning
	# the tunables and checking for new tunables.  We need to do this
	# test second because the reconfiguration script expects the
	# tunables to be partitioned already.
	#

	cd /etc/conf.v4

	unset RECONFIG

	CNT=0
	[ -d sdevice.d ] && CNT=`ls -1 sdevice.d | wc -l`
	[ $CNT != 0 ] && RECONFIG=1

	CNT=0
	[ -d sfsys.d ] && CNT=`ls -1 sfsys.d | wc -l`
	[ $CNT != 0 ] && RECONFIG=1

	[ "$RECONFIG" ] && {
	
		setup_reconfig
		return
	}

	#
	# Hopefully we won't get here too often, this is an expensive way
	# to decide if we should offer the user the option to reconfigure
	# their system.
	#
	# Now we need to see if any existing tunables need to have their
	# values tweaked.
	#

	while read TOKEN CURRENTv4 MINv4 MAXv4
	do
		#
		# First we need to see if the Version 4 default has been
		# overriden by a value in the Version 4 stune file.
		#

		grep "^$TOKEN[ 	]" /etc/conf.v4/cf.d/stune >/tmp/v4.$$

		[ $? = 0 ] && read JUNK CURRENTv4 </tmp/v4.$$

		# Then we'll get the current information for the new system

		$CONF/bin/idtune -g $TOKEN >/tmp/Dest.$$

		[ $? = 0 ] && read CURRENT DEFAULT MIN MAX JUNK </tmp/Dest.$$

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
		# shell does NOT differentiate between them in comparisons,
		# I need to convert them to a common base before comparison.
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

		[ $CONV_CURRv4 -gt $CONV_MAX ] && CONV_CURRv4=$CONV_MAX

		[ $CONV_CURRv4 -lt $CONV_MIN ] && CONV_CURRv4=$CONV_MIN

		#
		# In the script to merge the tunables, we'd use idtune here
		# to merge the value , but for our current needs, we'll
		# just offer them the reconfigure option and return.
		#

		[ $CONV_CURRv4 -gt $CONV_CURR ] && {

			setup_reconfig
			return
		}

		[ "$UPDEBUG" = "YES" ] && goany

	done <$UPINSTALL/tune.exist
}

[ "$INSTALL_TYPE" = "UPGRADE" ] && check_reconfig

# rm the lock and tmp files created in /tmp/upgtmp by up-n-over boot scripts
# rm files created by the upnover tools in $UPGRADE_STORE

rm -rf /tmp/upgtmp
rm -rf $UPGRADE_STORE/install_type $UPGRADE_STORE/*.env $UPGRADE_STORE/tmp

# create /etc/.release_ver to mark DESTiny machine

echo "$RELEASE	$VERSION" >$UNIX_REL

#
# We need to special case one file here.  If device.tab is restored
# along with the rest of the base system volatile files in the base
# pkg postinstall for an overlay, there are problems installing the
# remaining fnd set packages.
#
# For an UPGRADE, we're NOT going to "merge" it, because it's not
# clear exactly how to do it correctly now.  Rather than chance it,
# we'll document it.
#

# INSTALL_TYPE and AUTOMERGE are set in globals.sh

[ "$INSTALL_TYPE" = "OVERLAY" -a "$AUTOMERGE" = "Yes" ] && 
	mv -f $UPGRADE_STORE/etc/device.tab /etc/device.tab

############### End UPGRADE AND OVERLAY #################

menu_colors regular

[ ! -f ${RECONFIG_MARKER} ] &&
	menu -f ${HD_MENUS}/reboot.1 -o /tmp/results 2>/dev/null

[ "$UPDEBUG" = "YES" ] && goany
/sbin/rc6 reboot 1> /dev/sysmsg 2>&1 </dev/console
[ "$UPDEBUG" = "YES" ] && goany
