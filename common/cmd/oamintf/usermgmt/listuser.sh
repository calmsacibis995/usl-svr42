#!/bin/sh
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/listuser.sh	1.1.12.2"
#ident  "$Header: listuser.sh 2.0 91/07/12 $"
############################################################################
#	Module:  listuser.sh
#	Purpose: To display user friendly information on user(s)
#		 defined on the system.
############################################################################

ARGS=$*

# If privileged based system, check if the current user has
# privilege to use the logins command.  If not, exit gracefully.
if [ "$TFADMIN" = "/sbin/tfadmin" ]
then
	$TFADMIN -t logins || exit 1
fi

if [ "$ARGS" = "all" ]
then
	## logins -u option gives only non system reserved logins ##
	ARGS=`$TFADMIN logins -ou | cut -f1 -d:`
	ALL=true
fi
for i in $ARGS
do
   echo
   nstr=`$TFADMIN logins -oaxl${i} \
	 | sed -e "s/::/:undefined:/g" -e "s/: *:/:undefined:/g"`

   OFS=$IFS
   IFS=":" export IFS
   set $nstr

   if [ -z $1 ] 
   then
	echo "UX:listuser: ERROR: Cannot get information about users" >&2
	exit 1
   fi


   ### Get login name, uid, primary group name and id ###
   val=1
   while [ "$1" != "" ]
   do
	case $val in
	  1) user=$1;;
   	  2) uid=$1;;
   	  3) pgrp=$1;;
   	  4) pgid=$1;;
   	  5) comt=$1;;
   	  6) hdir=$1;;
   	  7) ush=$1;;
   	  8) pstat=$1;;
   	  9) dpsd=`echo $1 | sed "s/\([0-9]\{2\}\)\([0-9]\{2\}\)\([0-9]\{2\}\)/\1\/\2\/\3/"`;;
   	 10) min=$1;;
   	 11) max=$1;;
   	 12) warn=$1;;
   	 13) inact=$1;;
   	 14) exp=$1;;
       esac
       val=`expr $val + 1`
       shift
   done

   IFS=$OFS

   printf '    Login:   %-12s                      User ID: %6s\n' $user $uid
   printf '    Primary group:   %-8s         Primary group ID: %6s\n' $pgrp $pgid
   printf '    Comment:   %-25s' "$comt"


   ### Get password status ###
   case $pstat in
      PS) pstat="password"
          ### Get date when password was last changed ###
          echo "\n    Password status:   $pstat    Last changed on:   $dpsd"
          ;;
      LK) pstat="lock"
          echo "\n    Password Status:   $pstat";;
      NP) pstat="no password"
          echo "\n    Password Status:   $pstat";;
   esac
   

   ### Get home directory and user's shell ###
   printf '    Home Directory:   %-20s\n' $hdir
   printf '    Shell:   %-12s\n' $ush

   ### Display password aging information ###
   [ "$min" = "-1" ] || [ "$min" = "" ] && min="undefined" 
   echo "    Minimum number of days allowed between password changes:    $min"
   [ "$max" = "-1" ] || [ "$max" = "" ] && max="undefined" 
   echo "    Maximum number of days the password  is valid:   $max"
   [ "$warn" = "-1" ] || [ "$warn" = "" ] && warn="undefined" 
   echo "    Number of days for password warning message:   $warn"
   [ "$inact" = "-1" ] || [ "$inact" = "" ] && inact="undefined" 
   echo "    Number of days of login inactivity allowed:   $inact"
   case "$exp" in
      -1) echo "    Login expiration date:   undefined";;
       0) echo "    Login expiration date:   Never";;
      *)
  	epsd=`echo $exp | sed "s/\([0-9]\{1,2\}\)\([0-9]\{2\}\)\([0-9]\{2\}\)/\1\/\2\/\3/"`
	echo "    Login expiration date:   $epsd";;
   esac


   ## Display default audit mask if Auditing Utilities are installed ##
   nocount=`$TFADMIN /usr/bin/logins -ol${i} | count`
   if [ -d /var/sadm/pkg/audit ]
   then
	evnts=`$TFADMIN logins -obl${i}` 
	evcount=`echo $evnts | count`
 	echo "    Default audit event(s):  \c"
	if [ "$nocount" = "$evcount" ]
	then 
		echo "undefined"
	else
		OFS=$IFS
		IFS=:
		set $evnts
		IFS=$OFS
		shift 5
		echo "$*"
	fi
   fi
		
   ### If Enhanced Security Utilities are installed, display
   ### the security levels this user can access and the default
   ### level for this user.
   if [ -d /var/sadm/pkg/es ]
   then
####	echo "    Valid security level(s): `lvlget $i valid`"
	hstring=`$TFADMIN logins -ohl${i} | sed "s/::/:place:/g"`
	OFS=$IFS
	IFS=:
	set $hstring
	IFS=$OFS
	shift 5
	default="$1:$2"
	echo "    Valid security level(s): $1:$2 \c"
	shift 2
	while [ "$1" != "" ]
	do
		echo "$1:$2 \c"
		shift 2
	done
	echo "\n    Default login security level: `lvlget $i default"
   fi

   ## Were done for this user, send signal to coproc form Text.lsusr
   ## that we've produced a full record on a user.   This tells 
   ## Text.lsusr to go ahead and display next user.
   echo
   [ "$ALL" = "true" ] && echo ENDUSR  #send expectstring to Text.lsusr coproc
done

## Keep sending this to Text.lsusr frame until user cancels ##
while [ "$ALL" = "true" ]
do
 	echo "\n\n\t\tNo more users to list."
	echo "\t\tPress CANCEL to return to previous frame."
        echo ENDUSR
done

