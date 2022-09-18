#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:common/cmd/initpkg/rc1.sh	1.4.11.2"
#ident "$Header: rc1.sh 1.2 91/04/26 $"

#	"Run Commands" executed when the system is changing to init state 1
#

#	Turn privileges off in the working set. If any privilege is needed
#	by this script it will be turned on again. For the most part this
#	script only needs to pass privilege on to the startup scripts.
priv -allprivs work

. /etc/TIMEZONE
set `/sbin/who -r`
if [ $9 = "S" ]
then
	echo 'The system is coming up for administration.  Please wait.'
	BOOT=yes

elif [ $7 = "1" ]
then
	echo 'Changing to state 1.'
	if [ -d /etc/rc1.d ]
	then
		for f in /etc/rc1.d/K*
		{
			if [ -s ${f} ]
			then
				/sbin/sh ${f} stop
			fi
		}
	fi
fi

if [ -d /etc/rc1.d ]
then
	for f in /etc/rc1.d/S*
	{
		if [ -s ${f} ]
		then
			/sbin/sh ${f} start
		fi
	}
fi

# we know /usr is mounted because one of the rc1.d scripts calls mountall
/usr/sbin/killall 9

if [ "${BOOT}" = "yes" -a $7 = "1" ]
then
	echo 'The system is ready for administration.'
elif [ $7 = "1" ]
then
	echo 'Change to state 1 has been completed.'
fi
