#!SHELL
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/notify.sh	1.5.2.3"
#ident	"@(#)notify.sh	1.9 'attmail mail(1) command'"
#
#   NAME
#	notify - manage mail notification of users
#
#   SYNOPSIS
#	notify -y
#	notify -n
#	notify
#	notify -u user -o originator -s subject
#
#   DESCRIPTION
#
#	The notify program sets up asynchronous notification of
#	new incoming mail by doing a
#
#		'mail -F ">|NOTIFYPROG -o %R -s %S"
#
#	"notify -y" turns notification on for the user running the program.
#
#	"notify -n" turns notification off for the user running the program.
#
#	"notify" checks to see if notification is currently activated.
#
#   FILES
#	/var/mail/:forward/user - user forwarding file
#

PATH=REAL_PATH
export PATH
TMP=/tmp/notif$$
trap "rm -f $TMP" 0
NOTIFYPROG=/usr/lib/mail/notify2
YFLAG=0;
NFLAG=0;
SILENT=NO
if [ ! -t 1 ]; then
	# stdout not a tty. Be as silent as possible.....
	SILENT=YES
fi

CMD=`basename $0`

USEGETOPT set -- `getopt ynm: $*`
USEGETOPT if [ ${?} -ne 0 ]
USEGETOPT then set -- '-?'
USEGETOPT fi
USEGETOPT for arg
USEGETOPTS while getopts ynm: arg
do
USEGETOPT 	case "${arg}" in
USEGETOPTS 	case -"${arg}" in
	-n)	NFLAG=1
USEGETOPT 		shift
		;;
	-y)	YFLAG=1
USEGETOPT 		shift
		;;
	-m)	YFLAG=1		# ignore -m argument
USEGETOPT 		shift 2
		;;
	--)
USEGETOPT 		shift
		break ;;
	-\?)	if [ ${SILENT} = NO ]
		then
USEECHO 			echo "Usage: ${0} [-y|-n]" 1>&2
USEPFMT 			pfmt -l UX:$CMD -s action -g uxemail:447 "Usage: %s [-y|-n]\n" $0 1>&2
		fi
		exit 2
		;;
	esac
done
USEGETOPTS shift `expr $OPTIND - 1`

#
# If any args left, assume they 'forgot' the leading dash...:-)
#
if [ ${#} -gt 0 ]
then
	case "$1" in
	    [Yy]* ) YFLAG=1 ;;
	    [Nn]* ) NFLAG=1 ;;
	    * )
		if [ ${SILENT} = NO ]
		then
USEECHO 		    echo "Usage: ${0} [-y|-n]" 1>&2
USEPFMT 		    pfmt -l UX:$CMD -s action -g uxemail:447 "Usage: %s [-y|-n]\n" $0 1>&2
		fi
		exit 2
	esac
fi

# Get the name of the person from their id.
# id returns: uid=unum(uname) gid=gnum(gname) ...
set -- `id`
case "$1" in
    # Extract the name.
    uid=*'('*')' )	MYNAME=`expr "$1" : ".*(\(.*\))"` ;;
    # The user is no longer in the database? Exit
    * )
USEECHO 	echo "$0: No user name?" 1>&2
USEPFMT 	pfmt -l UX:$CMD -s error -g uxemail:448 "%s: No user name?\n" $0
	exit 1 ;;
esac

if [ ${NFLAG} -eq 1 -o ${YFLAG} -eq 0 ]
then
	if [ ! -f "FORWARDDIR/${MYNAME}" ]
	then
		if [ ${SILENT} = NO ]
		then
USEECHO 			echo "${0}: Mail notification not active"
USEPFMT 			pfmt -l UX:$CMD -s info -g uxemail:449 "Mail notification not active\n"
		fi
		exit 0
	fi
	read rest < FORWARDDIR/${MYNAME}
	MFILE=`expr "x${rest}" : "xForward to \(>|${NOTIFYPROG} -o %R -s %S\)"`
	if [ "x${MFILE}" = x ]
	then
		if [ ${SILENT} = NO ]
		then
USEECHO 			echo "${0}: Mail notification not active"
USEPFMT 			pfmt -l UX:$CMD -s info -g uxemail:449 "Mail notification not active\n"
		fi
		exit 0
	fi
	if [ ${NFLAG} -eq 1 ]
	then
		#
		# Turn notification facility off
		#
		mail -F "" > /dev/null 2>&1
		if [ ${SILENT} = NO ]
		then
USEECHO 			echo "${0}: Mail notification deactivated."
USEPFMT 			pfmt -l UX:$CMD -s info -g uxemail:450 "Mail notification deactivated.\n"
		fi
		exit 0
	fi
	#
	# Just report finding
	#
	if [ ${SILENT} = NO ]
	then
USEECHO 		echo "${0}: Mail notification active"
USEECHO 		echo "${0}: New mail messages will go to '${MFILE}'"
USEPFMT 		pfmt -l UX:$CMD -s info -g uxemail:451 "Mail notification active\n"
USEPFMT 		pfmt -l UX:$CMD -s info -g uxemail:452 "New mail messages will go to '%s'\n" ${MFILE}
	fi
	exit 0
fi
#
# Set up for notification.
#
if [ -f "FORWARDDIR/${MYNAME}" -a -s "FORWARDDIR/${MYNAME}" ]
then
	read rest < FORWARDDIR/${MYNAME}
	MFILE=`expr "x${rest}" : "xForward to \(>|${NOTIFYPROG} -o%R %S\)"`
	if [ "x${MFILE}" = x ]
	then
		if [ ${SILENT} = NO ]
		then
USEECHO 			echo "${0}: Notification cannot be installed unless FORWARDDIR/${MYNAME} is empty"
USEPFMT 			pfmt -l UX:$CMD -s info -g uxemail:453 "Notification cannot be installed unless %s/%s is empty\n" FORWARDDIR ${MYNAME}
		fi
		exit 0
	else
		if [ ${SILENT} = NO ]
		then
USEECHO 		    echo "${0}: Mail notification already installed."
USEPFMT 		    pfmt -l UX:$CMD -s info -g uxemail:454 "Mail notification already installed.\n"
		    exit 0
		fi
	fi
fi

#
# Just to be safe
#
if	MAIL=VAR_MAIL/${MYNAME} mail -F "" > /dev/null 2>&1
then	:
else
USEECHO 	echo "$0: Cannot install mail notification" 1>&2
USEPFMT 	pfmt -l UX:$CMD -s error -g uxemail:455 "Cannot install mail notification\n" 1>&2
	exit 2
fi
if  MAIL=VAR_MAIL/${MYNAME} \
	mail -F ">|${NOTIFYPROG} -o %R -s %S" > /dev/null 2>&1
then	:
else
USEECHO 	echo "$0: Cannot install mail notification" 1>&2
USEPFMT 	pfmt -l UX:$CMD -s error -g uxemail:455 "Cannot install mail notification\n" 1>&2
	exit 2
fi
if [ ${SILENT} = NO ]
then
USEECHO 	echo "${0}: Asynchronous 'new mail' notification installed"
USEPFMT 	pfmt -l UX:$CMD -s info -g uxemail:456 "Asynchronous 'new mail' notification installed\n"
fi
exit 0
