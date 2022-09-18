#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/fdscripts/common.sh	1.1.1.22"

# This script contains the common set of variable and function
# definitions used by all the shell scripts in fdscripts.

SCRIPTS=/etc/inst/scripts
HISTSIZE=1
HISTFILE=/tmp/.history
PATH=/sbin:/usr/bin:/etc
PATH=$PATH:/usr/sbin:/sbin:/usr/bin:/etc:/:/tmp
PATH=$PATH:/install/sbin:/install/usr/bin:/install/etc:/install/usr/sbin
PATH=$PATH:/etc/fs/s5
PATH=$PATH:/install/usr/lib/tape
LOCKFILE=/tmp/.mnt.lock;

export PATH SCRIPTS HISTSIZE HISTFILE

# clean up files being left behind by ksh here docs
rm -f /tmp/sh* > /dev/null 2>&1
[ -f $HISTFILE ] && {
      rm -f $HISTFILE > /dev/null 2>&1
}

logmsg() {
   echo "$1" >>$LOG
   case $2 in
      POWERDOWN)   	Do_shutdown;;
      HALT|SHELL)   	Create_Shell;;
   esac
}

error_out() {
    unset RETURN_VALUE
    ERRFILE=/etc/inst/locale/${LANG}/menus/fd/$1
    if [ ! -f $ERRFILE ] 
    then
	ERRFILE=/etc/inst/locale/C/menus/fd/$1
    fi
    cp ${FD_MENUS}/error.1 /tmp/error.menu

    while read j
    do
    echo $j >> /tmp/error.menu
    done < ${ERRFILE}

    menu_colors error
    menu -f /tmp/error.menu -o /dev/null 2>/dev/null
    [ "$UPDEBUG" = "YES" ] && goany
    logmsg "errored out $ERRFILE" POWERDOWN
}

Create_Shell () {
   cd /;
   exec sh
}

Do_shutdown () {
   sync
   sync
   Clean_Up
  uadmin 2 0 		#uadmin A_SHUTDOWN AD_HALT
}

Clean_Up() {
   cd /;
   rm -f /core ${LOCKFILE}
   Do_umount
}

Do_umount() {
   cd /
   umount $ROOT/home	> /dev/null 2>&1
   umount $ROOT/stand	> /dev/null 2>&1
   umount $ROOT/usr 	> /dev/null 2>&1
   umount $ROOT/tmp	> /dev/null 2>&1
   umount $ROOT/var	> /dev/null 2>&1
   umount $ROOT/home2	> /dev/null 2>&1
   umount /dev/dsk/0s1	> /dev/null 2>&1
   umount $ROOT		> /dev/null 2>&1
   sync 				> /dev/null 2>&1
   sync 				> /dev/null 2>&1
   sync 				> /dev/null 2>&1
}
set -a		# All environment variable sets exported
init_flags=""
ROOT=/mnt
USER=$ROOT/usr
STATUS=0
STRIKE="\nStrike ENTER when ready: \c"
ENTER_DEL="\nPlease strike ENTER when ready or DEL to cancel the installation: \c"
OFS=" 	"
BS=102400
INST_D=/
LOCALE_DIR=$INST_D/etc/inst/locale
LANG=C

#
#  If the menus dirs for the current ${LANG} do not exist, default
#  to the C-locale directory.
#
if [ -d ${LOCALE_DIR}/${LANG}/menus/hd ]
then
	HD_MENUS=${LOCALE_DIR}/${LANG}/menus/hd
else
	HD_MENUS=${LOCALE_DIR}/C/menus/hd
fi

if [ -d ${LOCALE_DIR}/${LANG}/menus/fd ]
then
	FD_MENUS=${LOCALE_DIR}/${LANG}/menus/fd
else
	FD_MENUS=${LOCALE_DIR}/C/menus/fd
fi

export FD_MENUS HD_MENUS

ETC=/etc
ETCINST=$ETC/inst
HISTORY=/tmp/history
LOG=$HISTORY/log
INST_BIN=$INST_D/sbin

DISK_SIZE=$HISTORY/Disk_Size
# ABSMIN_HARDDISK is the *absolute* minimum we'll let the hard disk
# be for installation.
# RECMIN_HARDDISK is the recommended minimum size for the UNIX partition
# that we'll enforce, if the hard disk is big enough.
ABSMIN_HARDDISK=50
RECMIN_HARDDISK=60
MIN_HARDDISK=${RECMIN_HARDDISK}
MIN_SECDISK=40
RELEASE="4.2"
VERSION="1"
UNIX_REL="/etc/.release_ver"
GLOBALS=/tmp/globals.sh
ROOTFSTYPE=/tmp/rootfstype

# The following variables are initialized here but may be overwritten
# in GLOBALS
INSTALL_TYPE=NEWINSTALL	
INSTALL_MODE=NULL
TERM=AT386-M
export TERM

# load in environment variables changed along the way -- these are
# kept in /tmp/globals.sh ($GLOBALS)

[ -f $GLOBALS ] && . $GLOBALS

# load in menu_colors.sh to pick up menu_colors function -- used to
# initialize background/foreground colors for different typs of
# screens
#
#  If the menu_colors.sh for the current ${LANG} does not exist, default
#  to the C-locale directory.
#
if [ -f ${LOCALE_DIR}/${LANG}/menus/menu_colors.sh ]
then
	. ${LOCALE_DIR}/${LANG}/menus/menu_colors.sh
else
	. ${LOCALE_DIR}/C/menus/menu_colors.sh
fi
