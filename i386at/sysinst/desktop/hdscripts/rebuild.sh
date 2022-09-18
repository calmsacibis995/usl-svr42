#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto:desktop/hdscripts/rebuild.sh	1.10.1.17"


Drv_Config ()
{
	Driver=$1
	IO=$2
	OnOff=$3

	# can't do anything if sdevice file isn't there!
	[ ! -f /etc/conf/sdevice.d/${Driver} ] && return;

	if [ "${OnOff}" = "On" ]
	then
	   # to turn the driver on for the I/O address spec'd
	   cat /etc/conf/sdevice.d/${Driver} | \
	     sed /${IO}/s/N/Y/ > /tmp/${Driver}.yy
	   cp /tmp/${Driver}.yy /etc/conf/sdevice.d/${Driver}
	else
	   # to turn the driver off for the I/O address spec'd
	   cat /etc/conf/sdevice.d/${Driver} | \
	     sed /${IO}/s/Y/N/ > /tmp/${Driver}.yy
	   cp /tmp/${Driver}.yy /etc/conf/sdevice.d/${Driver}
	fi
}

# main()
. ${SCRIPTS}/common.sh

#before rebuild...

# first determine if first serial port is configured
check_devs -s /dev/tty00
rc=$?
if [ "$rc" = "0" ] 
then
	# turn the driver on for the first serial port
	Drv_Config asyc 3f8 On
	# if a OEM wants to use asyhp instead of asyc, the line
	# above should be commented out and this one used in its
	# place
	# Drv_Config asyhp 3f8 On
else
	# We don't have one :-< Turn the driver off...
	Drv_Config asyc 3f8 Off
	# Drv_Config asyhp 3f8 Off
fi

# now determine if second serial port is configured
check_devs -s /dev/tty01
rc=$?
if [ "$rc" = "0" ] 
then
	# turn the driver on for the second serial port
	Drv_Config asyc 2f8 On
	# if a OEM wants to use asyhp instead of asyc, the line
	# above should be commented out and this one used in its
	# place
	# Drv_Config asyhp 2f8 On
else
	# We don't have one :-< Turn the driver off...
	Drv_Config asyc 2f8 Off
	# Drv_Config asyhp 3f8 Off
fi

#check for dump partition of either the first or second disk
/usr/sbin/prtvtoc /dev/rdsk/0s0 | /usr/bin/grep "6:	DUMP" > /dev/null 2>&1
if [ "$?" = "0" ];then
	ln /dev/dsk/0s6 /dev/dump >/dev/null 2>&1
	grep -v "PANICBOOT" < /etc/default/init > /tmp/TTT
	cat /tmp/TTT > /etc/default/init
	rm -rf /tmp/TTT
	echo "PANICBOOT=YES" >> /etc/default/init
	ed /etc/conf/sassign.d/kernel <<-! > /dev/null 2>&1
	/dump/
	s/\	2$/\	6/
	w
	q
	!
else
	/usr/sbin/prtvtoc /dev/rdsk/1s0 2>/dev/null | 
		/usr/bin/grep "6:	DUMP" > /dev/null 2>&1
	if [ "$?" = "0" ];then
		ln /dev/dsk/1s6 /dev/dump >/dev/null 2>&1
		grep -v "PANICBOOT" < /etc/default/init > /tmp/TTT
		cat /tmp/TTT > /etc/default/init
		rm -rf /tmp/TTT
		echo "PANICBOOT=YES" >> /etc/default/init
		ed /etc/conf/sassign.d/kernel <<-! > /dev/null 2>&1
		/dump/
		s/\	2$/\	22/
		w
		q
		!
	fi
fi

# echo "After pdimkdtab and mkdtab. Have a Shell"
# /sbin/sh

$IDCMD/idbuild -B 1>>/tmp/idbuild.out 2>&1
rc=$?
[ "$rc" != "0" ] && {
	menu_colors warn
	menu -f ${HD_MENUS}/rebuild.2 -o /tmp/response 2>/dev/null
}
rm -f /etc/.wsinitdate > /dev/null 2>&1
sync
sync
sync
menu -c
