#!SHELL
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/vacation2.sh	1.5.2.3"
#ident	"@(#)vacation2.sh	1.12 'attmail mail(1) command'"
#
# Second half of vacation(1).
# When new message arrives, save it in MAILFILE and check any prior messages
# from the same ORIGINATOR have been received. If so, exit. If not, record
# ORIGINATOR in LOGFILE and send canned message from MSGFILE to ORIGINATOR.
#
# Exit codes:
#	0 - note returned to author, mail(1) must save the message
#	1 - an error occurred
#	2 - mail was saved here, mail(1) should not save the message
#
PATH=REAL_PATH
export PATH
CMD=`basename $0`
ORIGINATOR=
MAILFILE=
LOGFILE=${HOME}/.maillog
MSGFILE=USR_SHARE_LIB/mail/std_vac_msg
SILENT=NO
FAILSAFE=
DAILY=NO
TMP=/tmp/.vac.$$

if [ ! -t 1 ]
then
	# stdout not a tty. Be as silent as possible.....
	SILENT=YES
fi

# parse the options
USEGETOPT set -- `getopt o:m:M:l:d $*`
USEGETOPT if [ ${?} -ne 0 ]
USEGETOPT then set -- '-?'
USEGETOPT fi
USEGETOPT 
USEGETOPT for arg
USEGETOPT do
USEGETOPT 	OPTARG="$2"
USEGETOPT 	case ${arg} in
USEGETOPTS while getopts o:m:M:l:d arg
USEGETOPTS do
USEGETOPTS 	case -"${arg}" in
	-o)	ORIGINATOR="$OPTARG"
USEGETOPT 		shift 2
		;;
	-m)	MAILFILE="$OPTARG"
USEGETOPT 		shift 2
		;;
	-M)	MSGFILE="$OPTARG"
USEGETOPT 		shift 2
		;;
	-l)	LOGFILE="$OPTARG"
USEGETOPT 		shift 2
		;;
	-d)	DAILY=YES
USEGETOPT 		shift
		;;
	--)   
USEGETOPT 		shift
		break ;;
	-\?)	case "$SILENT" in
		    NO )
USEECHO 			echo "Usage: ${0} -ooriginator [-m mailfile] [-M canned_msg_file] [-l logfile] [-d]" 1>&2
USEPFMT 			pfmt -l UX:$CMD -s action -g uxemail:463 "Usage: %s -ooriginator [-m mailfile] [-M canned_msg_file] [-l logfile] [-d]\n" $0 1>&2
			;;
		esac
		exit 1
		;;
	esac
done
USEGETOPTS shift `expr $OPTIND - 1`

if [ -z "${ORIGINATOR}" ]
then
	case "$SILENT" in
	    NO )
USEECHO 		echo "Usage: ${0} -ooriginator [-m mailfile] [-M canned_msg_file] [-l logfile] [-d]" 1>&2
USEPFMT 		pfmt -l UX:$CMD -s action -g uxemail:463 "Usage: %s -ooriginator [-m mailfile] [-M canned_msg_file] [-l logfile] [-d]\n" $0 1>&2
		;;
	esac
	exit 1
fi

# change user to uname!user; leave remote names alone
case "${ORIGINATOR}" in
    *!* ) ;;
    * ) ORIGINATOR=`mailinfo -s`!"${ORIGINATOR}" ;;
esac

# if $DAILY, use ~/.mailfile.day
case $DAILY in
    YES )
	if [ -n "$MAILFILE" ]
	then MAILFILE=$MAILFILE.`date +%m%d`
	else MAILFILE=$HOME/.mailfile.`date +%m%d`
	fi
	;;
esac

# append to the saved-mail file
ret=0
if [ -n "$MAILFILE" ]
then
	if cat >> "$MAILFILE"
	then ret=2
	else exit 1
	fi
fi

# have we seen this person before?
if fgrep -x ${ORIGINATOR} ${LOGFILE} > /dev/null 2>&1
then
	:	# yes we have
else
	# we haven't, so notify the originator
	echo ${ORIGINATOR} >> ${LOGFILE} 2>/dev/null
	# don't bother if there is a "Precedence: bulk"
	# or "Precedence: junk" header field.
	if egrep -i "^Precedence:[ 	]+(bulk|junk)" "${MSGFILE}" > /dev/null 2>&1
	then :
	else
		( trap "" 1 2 3 15; mail ${ORIGINATOR} < "${MSGFILE}" ) &
	fi
fi
exit $ret
