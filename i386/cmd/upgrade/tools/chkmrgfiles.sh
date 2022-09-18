#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)upgrade:i386/cmd/upgrade/tools/chkmrgfiles.sh	1.9"
#ident	"$Header: $"

Color_Console () {

   [ "$UPDEBUG" = "YES" ] && set -x

   TERM=AT386-M
   /usr/sbin/adpt_type >/dev/null
   DISPLAY=$?
   case $DISPLAY in
      0)  TERM=ANSI ;; #non-intergal console
      1|4)          ;; #1=MONO 4=VGA_MONO
      2|5|9|10)   #2=CGA 5=VGA_? 9=EGA 10=unknown controller
         TERM=AT386 
	 ;;
      3) #VGA_COLOR
         TERM=AT386   ;;
   esac
}

### main ()
#  script to display the screen:
#  
#  Do you want to auto merge config files or not
#
#  The arg is the name of the pkg
#  This script is invoked from  the request script of a pkg


#
#  Make sure LANG environment variable is set.  If it's not set
#  coming in to this request script, then default to the C-locale.
#
[ ${LANG} ] || LANG="C"
export LANG

#
#  If no ${LANG} directory, fall back on the C-locale.
#
if [ -d /etc/inst/locale/${LANG}/menus/upgrade ]
then
	UPGRADE_MSGS=/etc/inst/locale/${LANG}/menus/upgrade
else
	UPGRADE_MSGS=/etc/inst/locale/C/menus/upgrade
fi

SBINPKGINST=/usr/sbin/pkginst

. $SBINPKGINST/updebug

[ "$UPDEBUG" = "YES" ] && set -x

NAME="$1"

Color_Console
export TERM

#
#  Now invoke the menu program with everything we just extracted.
#

unset RETURN_VALUE

[ "$UPDEBUG" = "YES" ] && goany && set +x

menu -f ${UPGRADE_MSGS}/mergefiles.3 -o /tmp/response.$$ </dev/tty

[ "$UPDEBUG" = "YES" ] && set -x

. /tmp/response.$$
rm -f /tmp/response.$$
	
#	RETURN_VALUE=1 for Yes. Merge files
#	RETURN_VALUE=2 for No. Do not auto merge files.

rc=`expr $RETURN_VALUE - 1`
unset RETURN_VALUE

[ "$UPDEBUG" = "YES" ] && goany

exit  $rc
