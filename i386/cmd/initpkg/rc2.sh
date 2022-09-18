#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:i386/cmd/initpkg/rc2.sh	1.16.19.16"
#ident  "$Header: $"

#	"Run Commands" executed when the system is changing to init state 2,
#	traditionally called "multi-user".

#	Turn privileges off in the working set. If any privilege is needed
#	by this script it will be turned on again. For the most part this
#	script only needs to pass privilege on to the startup scripts.
priv -allprivs work

. /etc/TIMEZONE

#	Pickup start-up packages for mounts, daemons, services, etc.

if i386
then
	/sbin/modadmin -l sc01 st01 sw01 2>/dev/null
	if /etc/scsi/pdimkdev -f 2>/etc/scsi/dev.out
	then
		/etc/scsi/pdimkdtab -fi 2>/etc/scsi/dtab.out
	fi
	/sbin/modadmin -U sc01 st01 sw01 2>/dev/null
fi

set `/sbin/who -r`
if [ x$9 = "xS" -o x$9 = "x1" ]
then
	BOOT=yes

elif [ x$7 = "x2" ]
then
	echo 'Changing to state 2.'
	if [ -d /etc/rc2.d ]
	then
		for f in /etc/rc2.d/K*
		{
			if [ -s ${f} ]
			then
				/sbin/sh ${f} stop
			fi
		}
	fi
fi

if [ -d /etc/rc2.d ]
then
	for f in /etc/rc2.d/S*
	{
		if [ -s ${f} ]
		then
			/sbin/sh ${f} start
		fi
	}
fi

# Execute rc scripts from driver packages (386)
if [ -d /etc/idrc.d ]
then
	for f in /etc/idrc.d/*
	{
		if [ -s ${f} ]
		then
			/sbin/sh ${f}
		fi
	}
fi

if [ "${BOOT}" = "yes" -a -d /etc/rc.d ]
then
	for f in `/usr/bin/ls /etc/rc.d`
	{
		if [ ! -s /etc/init.d/${f} ]
		then
			/sbin/sh /etc/rc.d/${f} 
		fi
	}
fi

if [ x$9 = "xS" -o x$9 = "x1" ]
then
	if [ -f /etc/rc.d/PRESERVE ]	# historical segment for vi and ex
	then
		/usr/bin/mv /etc/rc.d/PRESERVE /etc/init.d
		/usr/bin/ln /etc/init.d/PRESERVE /etc/rc2.d/S02PRESERVE
	fi
fi

if [ "${BOOT}" = "yes" -a x$7 = "x2" ]
then
	# Enhanced Application Compatibility  Support
	/usr/bin/cat /etc/copyrights/*  2> /dev/null
	# End Enhanced Application Compatibility Support
elif [ x$7 = "x2" ]
then
	echo 'Change to state 2 has been completed.'
fi
