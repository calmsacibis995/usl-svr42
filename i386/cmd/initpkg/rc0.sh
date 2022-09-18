#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)initpkg:i386/cmd/initpkg/rc0.sh	1.15.20.9"
#ident "$Header: rc0.sh 1.2 91/06/28 $"

#	"Run Commands" for init state 0, 5 or 6
#	Leaves the system in a state where it is safe to turn off the power
#	or reboot. 
#
#	Takes an optional argument of off, firm or reboot
#	to specify if this is being run for init 0, init 5, or init 6.
#
#	In SVR4.0, inittab has been changed to no longer do the
#	uadmin to shutdown or enter firmware.  Instead, this script
#	is responsible.  By using an optional argument,
#	compatibility is maintained while still providing the needed
#	functionality to perform the uadmin call.

#	Turn privileges off in the working set. If any privilege is needed
#	by this script it will be turned on again. For the most part this
#	script only needs to pass privilege on to the startup scripts.

#


#	priv is not built in ksh, which is running  as sh when the system 
#	is overlayed.  Redirect stderr to /dev/null.

priv -allprivs work  2>/dev/null

#INFO messages should not appear in screen, save them in shut.log
/sbin/rm /var/adm/shut.log > /dev/null 2>&1
umask 022
echo 'The system is coming down.  Please wait.'

# make sure /usr is mounted before proceeding since init scripts
# and this shell depend on things on /usr file system
/sbin/mount /usr > /dev/null 2>&1
/sbin/initprivs 2> /dev/null

#	The following segment is for historical purposes.
#	There should be nothing in /etc/shutdown.d.
if [ -d /etc/shutdown.d ]
then
	for f in /etc/shutdown.d/*
	{
		if [ -s $f ]
		then
			/sbin/sh ${f} >> /var/adm/shut.log
		fi
	}
fi
#	End of historical section

if [ -d /etc/rc0.d ]
then
	for f in /etc/rc0.d/K*
	{
		if [ -s ${f} ]
		then
			/sbin/sh ${f} stop >> /var/adm/shut.log
		fi
	}

#	system cleanup functions ONLY (things that end fast!)	

	for f in /etc/rc0.d/S*
	{
		if [ -s ${f} ]
		then
			/sbin/sh ${f} start >> /var/adm/shut.log
		fi
	}
fi

# PC 6300+ Style Installation - execute shutdown scripts from driver packages
if [ -d /etc/idsd.d ]
then
	for f in /etc/idsd.d/*
	{
		if [ -s ${f} ]
		then
			/sbin/sh ${f} >> /var/adm/shut.log
		fi
	}
fi

trap "" 15
/usr/sbin/killall

/etc/conf/bin/idcpunix

/sbin/sync;/sbin/sync;/sbin/sync
rm -rf /tmp/*
/sbin/umountall > /dev/null 2>&1
/sbin/mount /var > /dev/null 2>&1
/sbin/sync;/sbin/sync;/sbin/sync

# check if user wants machine brought down or reboot
case "$1" in
	off)
		/sbin/umount /var > /dev/null 2>&1
		/sbin/uadmin 2 0
		;;

	firm)
		/sbin/umount /var > /dev/null 2>&1
		/sbin/uadmin 2 2
		;;

	reboot)
		/sbin/umount /var > /dev/null 2>&1
		/sbin/uadmin 2 1
		;;
esac

echo '
WARNING:  User level file systems may still be mounted.
	  Make sure to umount those file systems if you
	  are going to powerdown the system. Otherwise, 
	  the file systems maybe corrupted.\n'
