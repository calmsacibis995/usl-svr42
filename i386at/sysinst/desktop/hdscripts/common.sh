#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/common.sh	1.2.1.20"

# common defines in use by all the shell scripts in hdscripts

# clean up files being left behind by ksh here docs
[ -x /install/usr/bin/rm -o -x /usr/bin/rm ] && {
   rm -f /tmp/sh* > /dev/null 2>&1
   [ -f "$HISTFILE" ] && {
      rm -f $HISTFILE > /dev/null 2>&1
   }
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
   [ "$STTY" ] && { stty $STTY ; }
   exec sh
}

Do_shutdown () {
  Do_umount
  sync;sync;sync
  uadmin 2 0 		#uadmin A_SHUTDOWN AD_HALT
}

Do_umount() {
   [ "$STTY" ] && { stty $STTY ; }
   ROOT="/"
   cd $ROOT
   umount $ROOT/home	> /dev/null 2>&1
   umount $ROOT/stand	> /dev/null 2>&1
   umount $ROOT/usr 	> /dev/null 2>&1
   umount $ROOT/tmp	> /dev/null 2>&1
   umount $ROOT/var	> /dev/null 2>&1
   umount $ROOT/home2	> /dev/null 2>&1
   umount /dev/dsk/0s1	> /dev/null 2>&1
   sync 				> /dev/null 2>&1
   sync 				> /dev/null 2>&1
   sync 				> /dev/null 2>&1
}

PATH=$PATH:/usr/sbin:/sbin:/usr/bin:/etc:/usr/sbin
PATH=$PATH:/install/usr/lib/tape
export PATH
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

export HD_MENUS FD_MENUS

TAPE_DRIVE=$HISTORY/tape_drive
FND_SET=fnd
INST_D=/
INST_BIN=$INST_D/sbin


ETC=/etc
ETCINST=$ETC/inst
HISTORY=$ETCINST/history
LOG=$HISTORY/log
BASE_STORE=/base.save
FLOP_DRIVES=$HISTORY/flop_drives
TAPE_DRIVE=$HISTORY/tape_drive

CANTUPGRADE=/tmp/cantupgrade
INSTALL_MODE=NULL
TERM=AT386-M
export TERM 
INST_MODE=$ETCINST/inst_mode
desktop=No
IDCMD=/etc/conf/bin
ROOTFSTYPE=/tmp/rootfstype
GLOBALS="/tmp/globals.sh"
[ -f $GLOBALS ] && . $GLOBALS

INST=/usr/sadm/install/bin
export INST
[ ! -d $INST ] && {
	   mkdir -p $INST 2>/dev/null 1>&2
}

