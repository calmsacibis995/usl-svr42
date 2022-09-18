#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/loadhba.sh	1.15"

. ${SCRIPTS}/common.sh


# This function used to create a reverse dependency of
# the base package on the IHVHBA package just installed.
# This will prevent users from removing this package.
create_reverse_depend()
{
	name=$1
	PKGPATH=/var/sadm/pkg/${name}/install

	# if can't find directory, just skip
	[ ! -d ${PKGPATH} ] && return

	# create reverse dependency on the base package
	echo "R\tbase\tBase System" > ${PKGPATH}/depend
}


ReadHbaFloppy()
{
	HBA_NAME="$1"
	MENU_TYPE=regular
	HBA_ERROR=0
	export MENU_TYPE HBA_ERROR HBA_PROMPT

	menu_colors ${MENU_TYPE}
	menu -f ${HD_MENUS}/hba.scrn -o /dev/null < /dev/tty
	umount /dev/dsk/f0t 2>/dev/null
	umount /install	2>/dev/null

	while :
	do
		mntsts="1"
		ishba="1"
		mount -r /dev/dsk/f0t /install	2>/dev/null
		mntsts="$?"
		hbaformat="1"

		if [ "$mntsts" = "0" ] 
		then
			loadname=""
			if [ -f /install/etc/loadmods -a -f /install/etc/load.name ]
			then
				hbaformat="0"
				grep -v '^$' /install/etc/load.name > /tmp/loadname
				read loadname < /tmp/loadname
				if [ "$HBA_NAME" = "$loadname" ]
				then
					ishba="0"
					MEDIA=diskette1
				fi

				# Now look for package name so as to
				# create appropriate reverse depend
				# file on base package

				HERE=`pwd`
				cd /install
				> /tmp/ihvname.$$
				for i in *
				do
					[ -f $i/pkgmap ] && {
						echo $i >> /tmp/ihvname.$$
					}
				done
				cd ${HERE}
			fi
		else
			mntsts="1"
			[ -n "${HAVEFLOP2}" ] && {
			    mount -r /dev/dsk/f1t /install 2>/dev/null
			    mntsts="$?"
			}
			hbaformat="1"
			loadname=""
			if [ -f /install/etc/loadmods -a -f /install/etc/load.name ]
			then
				hbaformat="0"
				grep -v '^$' /install/etc/load.name > /tmp/loadname
				read loadname < /tmp/loadname
				if [ "$HBA_NAME" = "$loadname" ]
				then
					ishba="0"
					MEDIA=diskette2
				fi
				# Now look for package name so as to
				# create appropriate reverse depend
				# file on base package

				HERE=`pwd`
				cd /install
				> /tmp/ihvname.$$
				for i in *
				do
					[ -f $i/pkgmap ] && {
						echo $i >> /tmp/ihvname.$$
					}
				done
				cd ${HERE}
			fi
		fi
		umount /install	 2>/dev/null
		if [ "$ishba" = "0" ] 
		then
			echo "\033[2J\033[H"
			pkgadd -p -l -q -d ${MEDIA} all  < /dev/tty
			rc=$?
			[ "${rc}" = "0" -a -f /tmp/ihvname.$$ ] && {
				for i in `cat /tmp/ihvname.$$`
				do
				   create_reverse_depend $i
				done
				rm -f /tmp/ihvname.$$ 1>/dev/null 2>&1
				break;
			}
			HBA_ERROR=1
		else
		   if [ "$hbaformat" = "0" ] 
		   then
			HBA_ERROR=2
		   else
			HBA_ERROR=3
		   fi
		fi
		export HBA_ERROR
		MENU_TYPE=warn
		menu -f ${HD_MENUS}/hba.scrn -o  /dev/null < /dev/tty
		MENU_TYPE=regular
		HBA_ERROR=0
	done
}

[ ! -f /tmp/hivhba ] && exit;

sort -u /tmp/hivhba > /tmp/loaded.hbas
while read HBA_PROMPT
do
	ReadHbaFloppy "$HBA_PROMPT"
done <  /tmp/loaded.hbas

menu -f ${HD_MENUS}/hba.warn -o /dev/null < /dev/tty
