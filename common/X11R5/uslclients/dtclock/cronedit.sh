#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)dtclock:cronedit.sh	1.4"

case $1 in
-r)
    current=`crontab -l | grep -v $2`
    if [ "${current}" != "" ]
    then
       new="${current}\n$3"
    else
       new="$3"
    fi
    echo "${new}" | crontab &
    ;;
-a)
    current=`crontab -l`
    if [ "${current}" != "" ]
    then
       new="${current}\n$3"
    else
       new="$3"
    fi
    echo "${new}" | crontab &
    ;;
-d)
    current=`crontab -l | grep -v $2`
    if [ "${current}" != "" ]
    then
       echo "${current}" | crontab &
    else
       crontab -r &
    fi
    ;;
*)
    echo invalid option;;
esac
