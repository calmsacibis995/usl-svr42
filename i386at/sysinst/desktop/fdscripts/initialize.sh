#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/fdscripts/initialize.sh	1.2.1.28"

# perform initial operations for installation -- check hardware,
# show overview screen,  get/set time and timezone.

####### define functions first -- go to "main" for beginning ########

Introduction() {
   # Put up introduction screen
   menu_colors regular
   unset RETURN_VALUE
   menu -r -f ${FD_MENUS}/initial.1 -o /tmp/response 2>/dev/null
   return
}

Minimum_Hardware() {
    # Make sure integral hard disk is at least ABS_MINHARDDISK
    # big.  If the disk isn't as big as MIN_HARDDISK, modify
    # MIN_HARDDISK to be the size of the integral hard disk.
    HD_SZ=0
    HD_SZ=`partsize -s`
    rc=$?
    if [ "${rc}" != "0" -o  "$HD_SZ" -lt "$ABSMIN_HARDDISK" ]
    then
	menu_colors error
	unset RETURN_VALUE
	menu -f ${FD_MENUS}/initial.2 -o /tmp/response 2>/dev/null
	logmsg "Minimum Hardware configuration check failed. powerdown" POWERDOWN
     fi
     # The following code adjusts the MIN_HARDDISK constant
     # if the size of the disk is in the range between ABSMIN_HARDDISK
     # and MIN_HARDDISK.
     if [ ${HD_SZ} -lt ${MIN_HARDDISK} ]
     then
	# subtract 1 from HD_SZ when computing minimum, because
	# fdisk will want to save a cylinder for certain configs
	MIN_HARDDISK=`expr ${HD_SZ} - 1`
 	echo "MIN_HARDDISK=${MIN_HARDDISK}" >> ${GLOBALS}
	menu_colors warn
	unset RETURN_VALUE
	menu -f ${FD_MENUS}/initial.3 -o /tmp/response 2>/dev/null
     fi
     return
}

# We weren't sure whether there was color there or not.
# Ask the user. If it is color, set global foreground/background
# color. We only need to do this here, as stepper will have
# done it in the case where we were certain we had a color
# monitor.

Monitor_type() {
   unset RETURN_VALUE
   menu_colors regular
   menu -f ${FD_MENUS}/initial.4 -o /tmp/response 2>/dev/null
   . /tmp/response
   [ `expr ${RETURN_VALUE}` = 1 ] && {
   #returns 1 for color monitor, 2 for monochrome
      TERM=AT386
      # set forground/background colors
      echo "\033[0m\033[=0E\033[=7F\033[=1G\033[0m\033[J\033[7m\033[m"
      echo "\033[2J\033[H" # CLEAR the SCREEN
      return
   }
   TERM=AT386-M;
   return
}

# Determine adapter type, if VGA, determine if color or mono monitor
# set TERM appropriately. Ask about color if not sure whether color
# or mono there.

Color_Console () {
   TERM=AT386-M	# Assume mono by default
   adpt_type >/dev/null
   DISPLAY=$?
   case $DISPLAY in
      0)  TERM=ANSI export TERM
   	  echo "TERM=\"$TERM\"" >> $GLOBALS
          ;; #non-intergal console

      1|4) export TERM
   	  echo "TERM=\"$TERM\"" >> $GLOBALS
          ;; #1=MONO 4=VGA_MONO

      2|5|9|10)   #2=CGA 5=VGA_? 9=EGA 10=unknown controller
         TERM=AT386  export TERM
	 # ask about whether the user sees color
         Monitor_type     
   	 echo "TERM=\"$TERM\"" >> $GLOBALS
	 ;;

      3) #VGA_COLOR
         TERM=AT386 export TERM
   	 echo "TERM=\"$TERM\"" >> $GLOBALS
	 ;;
   esac

   # Force terminal into color 80x25 mode -- work around problem found
   # on Dell Fastwrite VGA controller
   [ "${TERM}" = "AT386" ] && stty VGA_C80x25 1>/dev/null 2>&1
}

# make sure directories are created, set up hard disk device links 
Initialize_Stuff () {

	for x in dsk rdsk
	do
	    cd /dev/$x/
	    for i in 0 1
	    do
		for j in 0 1 2 3 4 5 6 a b c d e f
		do
		   ln c0t${i}d0s$j ${i}s$j 1>/dev/null 2>&1
		done
		ln ${i}sa ${i}s10 1>/dev/null 2>&1
		ln ${i}sb ${i}s11 1>/dev/null 2>&1
		ln ${i}sc ${i}s12 1>/dev/null 2>&1
		ln ${i}sd ${i}s13 1>/dev/null 2>&1
		ln ${i}se ${i}s14 1>/dev/null 2>&1
		ln ${i}sf ${i}s15 1>/dev/null 2>&1
    	    done
	done
	ln /dev/dsk/0s1 /dev/root >/dev/null 2>&1
	ln /dev/rdsk/0s1 /dev/rroot >/dev/null 2>&1

	# set up so that if stepper is reentered from failed
	# upNover attempt to prompt for 2nd floppy, it uses
	# a new "hba.prompt2" that is really "bf2.again".

	[ -f /tmp/bf2.again ] && {
		mv /tmp/bf2.again /tmp/hba.prompt2
	}

	> ${GLOBALS}
	echo "\033[c"
	[ ! -d $ETC ] && mkdir $ETC
	[ ! -d $ETCINST ] && mkdir $ETCINST
	[ ! -d $HISTORY ] && mkdir $HISTORY
	rm -f /core ${LOCKFILE} /tmp/.[A-Z]*
	[  -f /etc/mnttab ] || > /etc/mnttab

	echo >/tmp/null
	>$LOG
}

#
#  Set the date and timezone (including the Daylight Savings stuff)
#
Time_N_Zone()
{
	
	. /etc/TIMEZONE
	/sbin/setclk 1>/dev/null 2>/dev/null
	#
	#  Get defaults to use so that the form the user is to fill out
	#  starts with the current time as default.
	#
	# run /etc/TIMEZONE to get 
	MINUTE=`date +"%M"`
	HOUR=`date +"%H"`
	DAY=`date +"%d"`
	MONTH=`date +"%m"`
	YEAR=`date +"%y"`

	export MINUTE HOUR DAY MONTH YEAR TIMEZONE DAYLIGHT WESTGMT

	#
	#  Set the current time and select timezone from N American
	#  standards.
	#  Sets: MINUTE, HOUR, DAY, MONTH, TIMEZONE
	#
	menu_colors regular
	menu -r -f ${FD_MENUS}/timezone.1 -o /tmp/tz.$$

	. /tmp/tz.$$

	#
	#  If we are in a known time zone, fill in the defaults for the
	#  Daylight Savings Code and # Hours west of GMT
	#
	case ${TIMEZONE} in
		"EST")
			WESTGMT=5
			DAYLIGHT="EDT"
			break;;
		"CST")
			WESTGMT=6
			DAYLIGHT="CDT"
			break;;
		"MST")
			WESTGMT=7
			DAYLIGHT="MDT"
			break;;
		"PST")
			WESTGMT=8
			DAYLIGHT="PDT"
			break;;
		*)
			unset TIMEZONE
			unset WESTGMT
			unset DAYLIGHT	;;
	esac

	#
	#  Now get full timezone information, including number of hours
	#  west of GMT and Daylight Savings Code name
	#  Sets TIMEZONE, WESTGMT, DAYLIGHT
	#  If we were given a known TZ above, don't bother with this.
	#
	[ ${TIMEZONE} ] || {
		menu_colors regular
		menu -r -f ${FD_MENUS}/timezone.2 -o /tmp/tz.$$
		. /tmp/tz.$$
	}

	#
	#  Set the system clock.  The 'date' command also sets the
	#  hardware's real time clock on the 386.
	#  If your hardware doesn't, please add code here to reset the
	#  machine's clock.
	#
	echo "TZ=${TIMEZONE}${WESTGMT}${DAYLIGHT}" >> /etc/TIMEZONE
	echo "export TZ" >> /etc/TIMEZONE
	
	. /etc/TIMEZONE

	date ${MONTH}${DAY}${HOUR}${MINUTE}${YEAR} >/dev/null 2>&1

	rm -f /tmp/tz.$$
}

# main()

. ${SCRIPTS}/common.sh

# This script will be reentered if an upgrade/overlay/interrupted
# installation was tried and failed. Make sure we don't do this
# twice.

[ -f "$CANTUPGRADE" ] && exit 0	#we came here the 2nd time

Initialize_Stuff

# read menu binary into memory so it runs fast on 1st invocation
OIFS=${IFS}
IFS=" "	# space
type menu | read a b pathname
echo ${pathname} | cpio -ocv > /dev/null 2>/dev/null
IFS=${OIFS}

Color_Console
Minimum_Hardware
Introduction

#
# Now do the date/time/timezone stuff
#
Time_N_Zone

OFS=" 	"
export RELEASE VERSION

echo "DISPLAY=\"$DISPLAY\"" >> $GLOBALS
echo "RELEASE=\"$RELEASE\"" >> $GLOBALS
echo "VERSION=\"$VERSION\"" >> $GLOBALS
echo "UNIX_REL=\"$UNIX_REL\"" >> $GLOBALS
