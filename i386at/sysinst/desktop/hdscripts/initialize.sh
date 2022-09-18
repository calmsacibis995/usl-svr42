#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/initialize.sh	1.3.1.19"


# main()


Copyright () {
   menu -c 2>/dev/null
   menu_colors regular
   menu -r -f ${HD_MENUS}/initial.2 -o /tmp/response 2>/dev/null
}

Welcome_Select_Mode() {
   INSTALL_MODE=AUTOMATIC
   unset RETURN_VALUE
   menu_colors regular
   menu -f ${HD_MENUS}/initial.5 -o /tmp/response 2>/dev/null
   . /tmp/response
   [ "`expr ${RETURN_VALUE}`" = "2" ] && {
   	INSTALL_MODE=CUSTOM
   }
}

Add_Flop_To_Device_Tab()
{
#$1 is drive number (1 or 2) $2 is the drive type (CMOS val)
#$3 is the /dev/dsk and /dev/rdsk name for the drive

	case $2 in

	1) DESC="5.25 inch (Low Density) Drive $1"
	   BLKS=702
	   DENS="mdens$1LOW"
	   BLKCYLS=18;;

	2) DESC="5.25 inch (High Density) Drive $1"
	   BLKS=2370
	   DENS="mdens$1HIGH"
	   BLKCYLS=30;;

	3) DESC="3.5 inch (Low Density) Drive $1"
	   BLKS=1422
	   DENS="mdens$1LOW"
	   BLKCYLS=18;;

	4) DESC="3.5 inch (High Density) Drive $1"
	   BLKS=2844
	   DENS="mdens$1HIGH"
	   BLKCYLS=36;;

	*) return 0;;

	esac

echo 'diskette'$1':/dev/rdsk/'$3':/dev/dsk/'$3'::desc="'${DESC}'" volume="diskette" capacity="'${BLKS}'" type="'${DENS}'" removable="true" display="false" copy="true" mkdtab="true" mkfscmd="/sbin/mkfs -Fs5 -b 512 /dev/rdsk/'$3 ${BLKS} 2 ${BLKCYLS}'" mountpt="/install" fmtcmd="/usr/sbin/format -i2 /dev/rdsk/'$3'"' >> /etc/device.tab

	echo "HAVEFLOP$1=$3" >> ${GLOBALS}

}

SaveFlopEntry()
{
# $1 is drive number (0 or 1 -- note we will use Drive 1/Drive 2 as
# user interface to be consistent w/OA&M labels for floppies
# $2 is the return val from check_devs, indicating what's in BIOS
# $3 is the file to which an entry for the floppy should be appended
	case $2 in

	1) DRIVE=f$1d9dt;;
	2) DRIVE=f$1q15dt;;
	3) DRIVE=f$13dt;;
	4) DRIVE=f$13ht;;
	*) return 0;;

	esac

	NUM=`expr $1 + 1`
	Add_Flop_To_Device_Tab $NUM $2 $DRIVE

}

CheckFloppyDrive()
{
	rm -f $FLOP_DRIVES $TAPE_DRIVE
	check_devs -f 1
	rc=$?
	SaveFlopEntry 0 $rc $FLOP_DRIVES
	check_devs -f 2 #second floppy
	rc=$?
	SaveFlopEntry 1 $rc $FLOP_DRIVES
	for i in /dev/rmt/tape?
	do
	   # use no-rewind device to keep microcassette
	   # from popping out...
	   if check_devs -t ${i}n
	   then
		echo $i >> ${TAPE_DRIVE}
	   fi
	done
}

# main()

. ${SCRIPTS}/common.sh
UNIX_REL=/etc/.release_ver
echo "UNIX_REL=\"$UNIX_REL\"" >>$GLOBALS

# drop temporary marker on disk in UNIX_REL to indicate to
# chkunixrel that interrupted install can now be done, as file
# systems have been created on the hard disk. This is done
# so that if power blows out, user does not have to sit
# through disk prep and can select to not install packages
# already installed. Only do this if INSTALL_TYPE is NEWINSTALL.

[ "${INSTALL_TYPE}" = "NEWINSTALL" ] && {
	echo "$RELEASE	PREINST" >$UNIX_REL
}

# recover from interrupted install after pdiconfig.sh or DEL'd install
# where knowledgeable user gets to shell and types exit 1.
# This code below sets device nodes in /dev back to their values
# before pdimkdev ran. Also recover vfstab and device.tab,
# which are also modified by pdiconfig.sh. Note that we use
# cp in case there are more interruptions.....

[ -r /dev/.ordsk/* ] && {
	rm -f /dev/rdsk/c* >/dev/null 2>&1
	cd /dev/.ordsk
	find . -print | cpio -pdumv /dev/rdsk >/dev/null 2>&1
	cd /
}

[ -r /dev/.odsk/* ] && {
	rm -f /dev/dsk/c* >/dev/null 2>&1
	cd /dev/.odsk
	find . -print | cpio -pdumv /dev/dsk >/dev/null 2>&1
	cd /
}

[ -r /dev/.ormt/* ] && {
	rm -f /dev/rmt/* >/dev/null 2>&1
	cd /dev/.ormt
	find . -print | cpio -pdumv /dev/rmt >/dev/null 2>&1
}

[ -r /etc/inst/.ovfstab ] && {
	cp /etc/inst/.ovfstab /etc/vfstab 1>/dev/null 2>/dev/null
}

[ -r /etc/inst/.odevice.tab ] && {
	cp /etc/inst/.odevice.tab /etc/device.tab 1>/dev/null 2>/dev/null
}

# All done recovering /dev nodes and related administrative files..

sync
sync
sync
stty sane
stty erase '^h' echoe -parity
echo " " >/tmp/menu.null
echo "\033[c"
STTY=`stty -g`
export STTY

# Prompt to remove 3rd floppy
Copyright

[ ! -d $ETC ] && mkdir $ETC
[ ! -d $ETCINST ] && mkdir $ETCINST
[ ! -d $HISTORY ] && mkdir $HISTORY
echo >/tmp/null
# mkdir /var/options and link /usr/options to it so that pkgadd's will work.
# pkgadd links some /var/sadm files to /usr/options.

if [ ! -d /var/options ] 
then
	# in case they are files
	rm -rf /var/options /usr/options 1>/dev/null 2>/dev/null
	# create directory and link 
	mkdir -p /var/options 1>/dev/null 2>/dev/null
	ln -s /var/options /usr/options 1>/dev/null 2>/dev/null
fi

# initialize device.tab for spool/standard entries.

echo '#ident	"@(#)proto:i386/at386/desktop/hdscripts/device.tab	1.3"'>/etc/device.tab
echo 'spool:::/usr/spool/pkg::desc="Packaging Spool Directory"'>>/etc/device.tab
CheckFloppyDrive
sync
sync
