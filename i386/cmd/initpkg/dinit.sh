#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:i386/cmd/initpkg/dinit.sh	1.2"
#ident  "$Header: $"

# 	Startup scripts that can be delayed to be run after login
#	processes have been started. Should run fast!

#	Turn privileges off in the working set. If any privilege is needed
#	by this script it will be turned on again. For the most part this
#	script only needs to pass privilege on to the startup scripts.
priv -allprivs work

#INFO messages should not appear in screen, save them in /var/adm/dinit.log
/sbin/rm /var/adm/dinit.log > /dev/null 2>&1

set `who -r`
if [ $9 = "S" -o $9 = "1" -o $9 = "?" ] #coming up
then
	if [ -d /etc/dinit.d ]
	then
		for f in /etc/dinit.d/K*
		{
			if [ -s ${f} ]
			then
				/sbin/sh ${f} stop >> /var/adm/dinit.log
			fi
		}

		for f in /etc/dinit.d/S*
		{
			if [ -s ${f} ]
			then
				/sbin/sh ${f} start >> /var/adm/dinit.log
			fi
		}
	fi
fi
