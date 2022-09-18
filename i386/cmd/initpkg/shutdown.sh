#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)initpkg:i386/cmd/initpkg/shutdown.sh	1.8.18.6"
#ident "$Header: shutdown.sh 1.4 91/07/08 $"

#	Sequence performed to change the init state of a machine.

#	This procedure checks to see if you are permitted and allows an
#	interactive shutdown.  The actual change of state, killing of
#	processes and such are performed by the new init state, say 0,
#	and its /sbin/rc0.

#	Usage:  shutdown [ -y ] [ -g<grace-period> ] [ -i<init-state> ]

priv -allprivs work	# turn off all working privileges

if [ `pwd` != / ]
then
	echo "$0:  You must be in the / directory to run /sbin/shutdown."
	exit 1
fi

# Make sure /usr is mounted
if [ ! -d /usr/bin ]
then
	echo "$0:  /usr is not mounted.  Mount /usr or use init to shutdown."
	exit 1
fi

if [ -r /etc/default/shutdown ]
then
	grace=`grep grace /etc/default/shutdown | cut -f2 -d=`
	if [ "${grace}" = "" ]
	then
		echo "$0:	Could not read /etc/default/shutdown."
		echo "	Setting default grace period to 60 seconds."
		grace=60
	fi
else
	echo "$0:	Could not read /etc/default/shutdown."
	echo "	Setting default grace period to 60 seconds."
	grace=60
fi

askconfirmation=yes

if i386
then
	initstate=0
else
	initstate=s
fi

while getopts ?yg:i: c
do
case $c in
	i)	initstate=$OPTARG; 
		case $initstate in
			2|3|4)
				echo "$0: Initstate $initstate is not for system shutdown."
				exit 1
			esac
		;;
	g)	grace=$OPTARG; 
		;;
	y)	askconfirmation=
		;;
	\?)	echo "Usage:  $0 [ -y ] [ -g<grace> ] [ -i<initstate> ]"
		exit 2
		;;
	*) 	
		echo "Usage:  $0 [ -y ] [ -g<grace> ] [ -i<initstate> ]"
		exit 2
		;;
	esac
done
shift `expr $OPTIND - 1`

if [ -x /usr/alarm/bin/event ]
then
	/usr/alarm/bin/event -c gen -e shutdown -- -t $grace
fi

if [ -z "${TZ}"  -a  -r /etc/TIMEZONE ]
then
	. /etc/TIMEZONE
fi

echo '\nShutdown started.    \c'
/usr/bin/date
echo

/sbin/sync

trap "exit 1"  1 2 15

a="`/sbin/who  |  /usr/bin/wc -l`"
if [ ${a} -gt 1  -a  ${grace} -gt 0 ]
then
	priv +macwrite +dacwrite +dev work
	/usr/sbin/wall<<-!
		The system will be shut down in ${grace} seconds.
		Please log off now.

	!
	priv -allprivs work
	/usr/bin/sleep ${grace}
	priv +macwrite +dacwrite +dev work
	/usr/sbin/wall <<-!
		THE SYSTEM IS BEING SHUT DOWN NOW ! ! !
		Log off now or risk your files being damaged.

	!
	priv -allprivs work
fi	

/usr/bin/sleep ${grace}

if [ ${askconfirmation} ]
then
	echo "Do you want to continue? (y or n):   \c"
	read b
else
	b=y
fi
if [ "$b" != "y" ]
then
	priv +macwrite +dacwrite +dev work
	/usr/sbin/wall <<-!
		False Alarm:  The system will not be brought down.
	!
	priv -allprivs work
	echo 'Shut down aborted.'
	exit 1
fi
case "${initstate}" in
s | S )
	priv +allprivs work
	. /sbin/rc0
	priv -allprivs work
esac

RET=0
priv +owner +compat work
/sbin/init ${initstate}
RET=$?
priv -allprivs work
exit ${RET}
