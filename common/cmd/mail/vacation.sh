#!SHELL
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/vacation.sh	1.5.2.3"
#ident	"@(#)vacation.sh	1.14 'attmail mail(1) command'"
#
# Set up vacation facility to send a canned message back to
# message originators alerting them to recipient's absense.
# Actualy installed by '/usr/bin/mail -F "|VACPROG -o %R [optional stuff]"
#
#
PATH=REAL_PATH
export PATH
TMP=/tmp/notif$$
VACPROG=/usr/lib/mail/vacation2
DEFLOGFILE=${HOME}/.maillog
DEFMSGFILE=USR_SHARE_LIB/mail/std_vac_msg
PROG=`basename $0`
MAILFILE=
LOGFILE=
MSGFILE=
SILENT=NO
DAILY=
FAILSAFE=
TODAY=
FORWARD=
EXITCODES="C=0;S=2;F=*;"
REMOVE=
if [ ! -t 1 ]
then
	# stdout not a tty. Be as silent as possible.....
	SILENT=YES
fi

USEGETOPT set -- `getopt m:M:l:F:df:ni: $*`
USEGETOPT if [ ${?} -ne 0 ]
USEGETOPT then set -- '-?'
USEGETOPT fi

USEGETOPT for arg
USEGETOPT do
USEGETOPT 	OPTARG="$2"
USEGETOPT 	case $arg in
USEGETOPTS while getopts m:M:l:F:f:dni: arg
USEGETOPTS do
USEGETOPTS 	case -"$arg" in
	-m)	MAILFILE="$OPTARG"
USEGETOPT 		shift 2
		;;
	-M)	MSGFILE="$OPTARG"
USEGETOPT 		shift 2
		;;
	-l)	LOGFILE="$OPTARG"
USEGETOPT 		shift 2
		;;
	-F)		# ignore -F
USEGETOPT 		shift 2
		;;
	-f)	FORWARD="$FORWARD $OPTARG";
USEGETOPT 		shift 2
		;;
	-i)	FORWARD="$FORWARD $OPTARG"; EXITCODES="S=0,2;F=*;";
USEGETOPT 		shift 2
		;;
	-d)	DAILY=.date; TODAY=.`date +%m%d`;
		[ -z "$MAILFILE" ] && MAILFILE=$HOME/.mailfile
USEGETOPT 		shift
		;;
	-n)	REMOVE=yes
USEGETOPT 		shift
		;;
	--)
USEGETOPT 		shift
		break ;;
	-\?)	case ${SILENT} in
		    NO )
USEECHO 			echo "Usage: ${0} [-M canned_msg_file] [-l logfile] [-m savefile] [-d] [-f forwarding-id]" 1>&2
USEECHO 			echo "\t$0 -n" 1>&2
USEPFMT 			pfmt -l UX:$PROG -s action -g uxemail:494 "Usage: %s [-M canned_msg_file] [-l logfile] [-m savefile] [-d] [-f forwarding-id] [-i forwarding-id]\n" $0 1>&2
USEPFMT 			pfmt -l UX:$PROG -s action -g uxemail:495 "\t%s -n\n" $0 1>&2
			;;
		esac
		exit 1
		;;
	esac
done
USEGETOPTS shift `expr $OPTIND - 1`

if [ -n "$REMOVE" ]
then
	if [ -n "$MAILFILE" -o -n "$MSGFILE" -o -n "$LOGFILE" -o -n "$DAILY" ]
	then
USEECHO 		echo "$0: cannot use -n with other options" 1>&2
USEPFMT 		pfmt -l UX:$PROG -s error uxemail:496 "%s: cannot use -n with other options\n" $0 1>&2
		exit 1
	else
		mail -F ""
		exit 0
	fi
fi

[ -z "$MSGFILE" ] && MSGFILE=${DEFMSGFILE}
[ -z "$LOGFILE" ] && LOGFILE=${DEFLOGFILE}

if [ -f "$LOGFILE" -a ! -w "$LOGFILE" ]
then
USEECHO 	echo "$0: Cannot write to $LOGFILE" 1>&2
USEPFMT 	pfmt -l UX:$PROG -s error -g uxemail:458 "Cannot write to %s\n" $LOGFILE 1>&2
	exit 1
fi
# Reset LOGFILE
if { > "$LOGFILE"; } 2>/dev/null
then :
else
USEECHO 	echo "$0: Cannot write to $LOGFILE" 1>&2
USEPFMT 	pfmt -l UX:$PROG -s error -g uxemail:458 "Cannot write to %s\n" $LOGFILE 1>&2
	exit 1
fi
if [ -f "$LOGFILE" -a ! -r "$LOGFILE" ]
then
USEECHO 	echo "$0: Cannot read $LOGFILE" 1>&2
USEPFMT 	pfmt -l UX:$PROG -s error -g uxemail:459 "Cannot read %s\n" $LOGFILE 1>&2
	exit 1
fi
if [ -n "$MAILFILE" -a -f "$MAILFILE$TODAY" -a ! -w "$MAILFILE$TODAY" ]
then
USEECHO 	echo "$0: Cannot write to $MAILFILE$TODAY" 1>&2
USEPFMT 	pfmt -l UX:$PROG -s error -g uxemail:458 "Cannot write to %s\n" "$MAILFILE$TODAY" 1>&2
	exit 1
fi

# build up the command line
CMD="mail -F \"$FORWARD | $EXITCODES ${VACPROG} -o %R"

if [ -n "${MAILFILE}" ]
then
	CMD="${CMD} -m ${MAILFILE}"
fi
if [ "x${LOGFILE}" != "x$DEFLOGFILE" ]
then
	CMD="${CMD} -l ${LOGFILE}"
fi
if [ "x${MSGFILE}" != "x$DEFMSGFILE" ]
then
	CMD="${CMD} -M ${MSGFILE}"
fi
if [ -n "$FAILSAFE" ]
then
	CMD="${CMD} -F ${FAILSAFE}"
fi
if [ -n "$DAILY" ]
then
	CMD="${CMD} -d"
fi
CMD="${CMD}\" > $TMP 2>&1"
#
# Just to be safe
#
if  mail -F ""  > $TMP 2>&1
then	:
else
	cat $TMP
	rm -f $TMP
	exit 2
fi
# Create the file in advance
if [ -n "${MAILFILE}" ]
then
	if [ ! -f "${MAILFILE}${TODAY}" ]
	then
		> ${MAILFILE}${TODAY}
		chmod 600 ${MAILFILE}${TODAY}
	fi
fi

eval ${CMD}
if [ ${?} -ne 0 ]
then
	cat $TMP
	rm -f $TMP
	exit 2
fi
case ${SILENT} in
    NO )
USEECHO 	echo "${0}: Vacation notification installed"
USEPFMT 	pfmt -l UX:$PROG -s info -g uxemail:460 "Vacation notification installed\n"
	if [ -n "${MAILFILE}" ]
	then
USEECHO 		echo "${0}: New mail messages will go to '${MAILFILE}.date'"
USEPFMT 		pfmt -l UX:$PROG -s info -g uxemail:461 "New mail messages will go to '%s'\n" "${MAILFILE}.date"
	fi
USEECHO 	echo "${0}: Logging will go to '${LOGFILE}${DAILY}'"
USEECHO 	echo "${0}: '${MSGFILE}' will be used for the canned message"
USEPFMT 	pfmt -l UX:$PROG -s info -g uxemail:462 "Logging will go to '%s%s'\n" "${LOGFILE}" "${DAILY}"
USEPFMT 	pfmt -l UX:$PROG -s info -g uxemail:476 "'%s' will be used for the canned message\n" "${MSGFILE}"
	;;
esac
rm -f $TMP
exit 0
